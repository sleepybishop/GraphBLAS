//------------------------------------------------------------------------------
// GB_add_phase2: C=A+B, C<M>=A+B, or C<!M>=A+B
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// GB_add_phase2 computes C=A+B, C<M>=A+B, or C<!M>=A+B.  It is preceded first
// by GB_add_phase0, which computes the list of vectors of C to compute (Ch)
// and their location in A and B (C_to_[AB]).  Next, GB_add_phase1 counts the
// entries in each vector C(:,j) and computes Cp.

// GB_add_phase2 computes the pattern and values of each vector of C(:,j),
// fully in parallel.

// C, M, A, and B can be standard sparse or hypersparse, as determined by
// GB_add_phase0.  All cases of the mask M are handled: not present, present
// and not complemented, and present and complemented.

// This function either frees Cp and Ch, or transplants then into C, as C->p
// and C->h.  Either way, the caller must not free them.

// op may be NULL.  In this case, the intersection of A and B must be empty.
// This is used by GB_wait only, for merging the pending tuple matrix T into A.

// PARALLEL: done, except for the last phase, to prune empty vectors from C,
// if it is hypersparse with empty vectors.  Takes O(C->nvec time), and is
// not always used.

#include "GB.h"
#ifndef GBCOMPACT
#include "GB_binop__include.h"
#endif

GrB_Info GB_add_phase2      // C=A+B, C<M>=A+B, or C<!M>=A+B
(
    GrB_Matrix *Chandle,    // output matrix (unallocated on input)
    const GrB_Type ctype,   // type of output matrix C
    const bool C_is_csc,    // format of output matrix C
    const GrB_BinaryOp op,  // op to perform C = op (A,B), or NULL if no op

    // from GB_add_phase1
    const int64_t *restrict Cp,         // vector pointers for C
    const int64_t Cnvec_nonempty,       // # of non-empty vectors in C

    // analysis from GB_add_phase0:
    const int64_t Cnvec,
    const int64_t max_Cnvec,
    const int64_t *restrict Ch,
    const int64_t *restrict C_to_A,
    const int64_t *restrict C_to_B,
    const bool Ch_is_Mh,        // if true, then Ch == M->h

    // original input to GB_add
    const GrB_Matrix M,         // optional mask, may be NULL
    const bool Mask_comp,
    const GrB_Matrix A,
    const GrB_Matrix B,
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (Cp != NULL) ;
    ASSERT_OK_OR_NULL (GB_check (op, "op for add phase2", GB0)) ;
    ASSERT_OK (GB_check (A, "A for add phase2", GB0)) ;
    ASSERT_OK (GB_check (B, "B for add phase2", GB0)) ;
    ASSERT_OK_OR_NULL (GB_check (M, "M for add phase2", GB0)) ;
    ASSERT (A->vdim == B->vdim) ;

    if (op == NULL)
    { 
        // GB_wait does no typecasting.  A and T have the same type when
        // computing A=A+T, and no operator is used since A and T have disjoint
        // nonzero patterns.  No mask is used.
        ASSERT (ctype == A->type) ;
        ASSERT (ctype == B->type) ;
        ASSERT (M == NULL) ;
    }
    else
    { 
        ASSERT (GB_Type_compatible (ctype,   A->type)) ;
        ASSERT (GB_Type_compatible (ctype,   B->type)) ;
        ASSERT (GB_Type_compatible (ctype,   op->ztype)) ;
        ASSERT (GB_Type_compatible (A->type, op->xtype)) ;
        ASSERT (GB_Type_compatible (B->type, op->ytype)) ;
    }

    //--------------------------------------------------------------------------
    // determine the number of threads to use
    //--------------------------------------------------------------------------

    GB_GET_NTHREADS (nthreads, Context) ;
    // TODO reduce nthreads for small problem (work: about O(anz+bnz+Cnvec))

    //--------------------------------------------------------------------------
    // allocate the output matrix C
    //--------------------------------------------------------------------------

    int64_t cnz = Cp [Cnvec] ;
    (*Chandle) = NULL ;

    // C is hypersparse if both A and B are (contrast with GrB_Matrix_emult),
    // or if M is present, not complemented, and hypersparse.
    // C acquires the same hyperatio as A.

    bool C_is_hyper = (Ch != NULL) ;

    // allocate the result C (but do not allocate C->p or C->h)
    GrB_Info info ;
    GrB_Matrix C = NULL ;           // allocate a new header for C
    GB_CREATE (&C, ctype, A->vlen, A->vdim, GB_Ap_null, C_is_csc,
        GB_SAME_HYPER_AS (C_is_hyper), A->hyper_ratio, Cnvec, cnz, true,
        Context) ;
    if (info != GrB_SUCCESS)
    { 
        // out of memory; caller must free C_to_A, C_to_B
        GB_FREE_MEMORY (Cp, GB_IMAX (2, Cnvec+1), sizeof (int64_t)) ;
        GB_FREE_MEMORY (Ch, max_Cnvec, sizeof (int64_t)) ;
        return (info) ;
    }

    // add Cp as the vector pointers for C, from GB_add_phase1
    C->p = (int64_t *) Cp ;

    // add Ch as the the hypersparse list for C, from GB_add_phase0
    if (C_is_hyper)
    { 
        C->h = (int64_t *) Ch ;
        C->nvec = Cnvec ;
    }

    // now Cp and Ch have been transplanted into C, so they must not be freed.

    C->nvec_nonempty = Cnvec_nonempty ;
    C->magic = GB_MAGIC ;
    GB_Type_code ccode = ctype->code ;

    //--------------------------------------------------------------------------
    // using a built-in binary operator
    //--------------------------------------------------------------------------

    bool done = false ;

#ifndef GBCOMPACT

    //--------------------------------------------------------------------------
    // define the worker for the switch factory
    //--------------------------------------------------------------------------

    #define GB_AaddB(mult,xyname) GB_AaddB_ ## mult ## xyname

    #define GB_BINOP_WORKER(mult,xyname)                            \
    {                                                               \
        GB_AaddB(mult,xyname) (C, M, Mask_comp, A, B, Ch_is_Mh,     \
            C_to_A, C_to_B, nthreads) ;                             \
        done = true ;                                               \
    }                                                               \
    break ;

    //--------------------------------------------------------------------------
    // launch the switch factory
    //--------------------------------------------------------------------------

    GB_Opcode opcode ;
    GB_Type_code xycode, zcode ;

    if (GB_binop_builtin (A, false, B, false, op,
        false, &opcode, &xycode, &zcode) && ccode == zcode)
    { 
        #include "GB_binop_factory.c"
        ASSERT (done) ;
    }

#endif

    //--------------------------------------------------------------------------
    // generic worker
    //--------------------------------------------------------------------------

    if (!done)
    {

        GxB_binary_function fadd ;
        size_t csize, asize, bsize, xsize, ysize, zsize ;
        GB_cast_function
            cast_A_to_X, cast_B_to_Y, cast_A_to_C, cast_B_to_C, cast_Z_to_C ;

        if (op == NULL)
        { 
            // implicit GB_SECOND_[type] operator with no typecasting
            fadd = NULL ;
            csize = ctype->size ;
            asize = csize ;
            bsize = csize ;
            xsize = csize ;
            ysize = csize ;
            zsize = csize ;
            cast_A_to_X = GB_copy_user_user ;
            cast_B_to_Y = GB_copy_user_user ;
            cast_A_to_C = GB_copy_user_user ;
            cast_B_to_C = GB_copy_user_user ;
            cast_Z_to_C = GB_copy_user_user ;
        }
        else
        { 
            // normal case, C = A + B with optional typecasting
            fadd = op->function ;
            csize = ctype->size ;
            asize = A->type->size ;
            bsize = B->type->size ;
            xsize = op->xtype->size ;
            ysize = op->ytype->size ;
            zsize = op->ztype->size ;
            cast_A_to_X = GB_cast_factory (op->xtype->code, A->type->code) ;
            cast_B_to_Y = GB_cast_factory (op->ytype->code, B->type->code) ;
            cast_A_to_C = GB_cast_factory (ccode,           A->type->code) ;
            cast_B_to_C = GB_cast_factory (ccode,           B->type->code) ;
            cast_Z_to_C = GB_cast_factory (ccode,           op->ztype->code) ;
        }

        // C(i,j) = (ctype) A(i,j), located in Ax [pA]
        #define GB_COPY_A_TO_C(cij,Ax,pA)                                   \
            cast_A_to_C (cij, Ax +((pA)*asize), asize) ;

        // C(i,j) = (ctype) B(i,j), located in Bx [pB]
        #define GB_COPY_B_TO_C(cij,Bx,pB)                                   \
            cast_B_to_C (cij, Bx +((pB)*bsize), bsize) ;

        // aij = (xtype) A(i,j), located in Ax [pA]
        #define GB_GETA(aij,Ax,pA)                                          \
            GB_void aij [xsize] ;                                           \
            cast_A_to_X (aij, Ax +((pA)*asize), asize) ;

        // bij = (ytype) B(i,j), located in Bx [pB]
        #define GB_GETB(bij,Bx,pB)                                          \
            GB_void bij [ysize] ;                                           \
            cast_B_to_Y (bij, Bx +((pB)*bsize), bsize) ;

        // C(i,j) = (ctype) (A(i,j) + B(i,j))
        // not used if op is null
        #define GB_BINOP(cij, aij, bij)                                     \
            ASSERT (op != NULL) ;                                           \
            GB_void z [zsize] ;                                             \
            fadd (z, aij, bij) ;                                            \
            cast_Z_to_C (cij, z, csize) ;

        // address of Cx [p]
        #define GB_CX(p) Cx +((p)*csize)

        #define GB_ATYPE GB_void
        #define GB_BTYPE GB_void
        #define GB_CTYPE GB_void

        #define GB_PHASE_2_OF_2

        #include "GB_add_template.c"

    }

    //--------------------------------------------------------------------------
    // prune empty vectors from Ch
    //--------------------------------------------------------------------------

    // TODO make this a function; see also GB_emult, GB_delete_zombies
    if (C_is_hyper && Cnvec_nonempty < Cnvec)
    {
        int64_t *restrict Cp = C->p ;
        int64_t *restrict Ch = C->h ;
        int64_t cnvec_new = 0 ;

        // TODO this loop is sequential.  Could use a parallel cumulative sum
        // of the Cp > 0 condition, and then an out-of-place copy to new Ch and
        // Cp arrays.
        for (int64_t k = 0 ; k < Cnvec ; k++)
        {
            int64_t cjnz = Cp [k+1] - Cp [k] ;
            if (cjnz > 0)
            { 
                Cp [cnvec_new] = Cp [k] ;
                Ch [cnvec_new] = Ch [k] ;
                cnvec_new++ ;
            }
        }

        Cp [cnvec_new] = Cp [Cnvec] ;
        C->nvec = cnvec_new ;
        ASSERT (cnvec_new == Cnvec_nonempty) ;
        // reduce the size of Cp and Ch (this cannot fail)
        bool ok ;
        GB_REALLOC_MEMORY (C->p, cnvec_new, GB_IMAX (2, Cnvec+1),
            sizeof (int64_t), &ok) ;
        ASSERT (ok) ;
        GB_REALLOC_MEMORY (C->h, cnvec_new, max_Cnvec, sizeof (int64_t), &ok) ;
        ASSERT (ok) ;
        C->plen = cnvec_new ;
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    // caller must free C_to_A, and C_to_B, but not Cp or Ch
    ASSERT_OK (GB_check (C, "C output for add phase2", GB0)) ;
    (*Chandle) = C ;
    return (GrB_SUCCESS) ;
}

