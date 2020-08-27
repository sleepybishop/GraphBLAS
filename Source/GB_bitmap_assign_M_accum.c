//------------------------------------------------------------------------------
// GB_bitmap_assign_M_accum:  assign to C bitmap 
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------
// C<M>(I,J) += A       assign
// C(I,J)<M> += A       subassign

// C<M,repl>(I,J) += A       assign
// C(I,J)<M,repl> += A       subassign
//------------------------------------------------------------------------------

// C:           bitmap
// M:           present, hypersparse or sparse (not bitmap or full)
// Mask_comp:   false
// Mask_struct: true or false
// C_replace:   true or false
// accum:       present
// A:           matrix (hyper, sparse, bitmap, or full), or scalar
// kind:        assign, row assign, col assign, or subassign

#include "GB_bitmap_assign_methods.h"

#define GB_FREE_ALL \
    GB_ek_slice_free (&pstart_Mslice, &kfirst_Mslice, &klast_Mslice) ;

GrB_Info GB_bitmap_assign_M_accum
(
    // input/output:
    GrB_Matrix C,               // input/output matrix in bitmap format
    // inputs:
    const bool C_replace,       // descriptor for C
    const GrB_Index *I,         // I index list
    const int64_t nI,
    const int Ikind,
    const int64_t Icolon [3],
    const GrB_Index *J,         // J index list
    const int64_t nJ,
    const int Jkind,
    const int64_t Jcolon [3],
    const GrB_Matrix M,         // mask matrix, which is not NULL here
//  const bool Mask_comp,       // false here
    const bool Mask_struct,     // true if M is structural, false if valued
    const GrB_BinaryOp accum,   // present here
    const GrB_Matrix A,         // input matrix, not transposed
    const void *scalar,         // input scalar
    const GrB_Type scalar_type, // type of input scalar
    const int assign_kind,      // row assign, col assign, assign, or subassign
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GBURBLE_BITMAP_ASSIGN ("bit3", M, false, accum) ;
    ASSERT (GB_IS_BITMAP (C)) ;
    ASSERT (GB_IS_HYPERSPARSE (M) || GB_IS_SPARSE (M)) ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign, M, accum", GB0) ;
    ASSERT_MATRIX_OK (M, "M for bitmap assign, M, accum", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (A, "A for bitmap assign, M, accum", GB0) ;

    //--------------------------------------------------------------------------
    // get C, M, A, and accum
    //--------------------------------------------------------------------------

    GB_GET_C
    GB_SLICE_M
    GB_GET_A
    GB_GET_ACCUM

    //--------------------------------------------------------------------------
    // do the assignment
    //--------------------------------------------------------------------------

    if (A == NULL && assign_kind == GB_SUBASSIGN)
    {

        //----------------------------------------------------------------------
        // scalar subassignment: C(I,J)<M> += scalar
        //----------------------------------------------------------------------

        ASSERT (assign_kind == GB_SUBASSIGN) ;
        int64_t keep = C_replace ? 3 : 1 ;

        // for all entries in the mask M:
        #undef  GB_MASK_WORK
        #define GB_MASK_WORK(pC)            \
        {                                   \
            int8_t cb = Cb [pC] ;           \
            if (cb == 0)                    \
            {                               \
                /* Cx [pC] = scalar */      \
                GB_ASSIGN_SCALAR (pC) ;     \
                cnvals++ ;                  \
            }                               \
            else /* (cb == 1) */            \
            {                               \
                /* Cx [pC] += scalar */     \
                GB_ACCUM_SCALAR (pC) ;      \
            }                               \
            /* keep this entry */           \
            Cb [pC] = keep ;                \
        }
        #include "GB_bitmap_assign_M_sub_template.c"

        if (C_replace)
        {
            // for all entries in IxJ
            #undef  GB_IXJ_WORK
            #define GB_IXJ_WORK(pC)             \
            {                                   \
                int8_t cb = Cb [pC] ;           \
                if (cb == 1)                    \
                {                               \
                    /* delete this entry */     \
                    Cb [pC] = 0 ;               \
                    cnvals-- ;                  \
                }                               \
                else                            \
                {                               \
                    /* clear the mask from C */ \
                    /* 0: no change */          \
                    /* 1: see above */          \
                    /* 2: to zero */            \
                    /* 3: keep, to 1 */         \
                    Cb [pC] = (cb == 3) ;       \
                }                               \
            }
            #include "GB_bitmap_assign_IxJ_template.c"
        }

    }
    else
    {

        //----------------------------------------------------------------------
        // scatter M into C
        //----------------------------------------------------------------------

        #undef  GB_MASK_WORK
        #define GB_MASK_WORK(pC) Cb [pC] += 2
        #include "GB_bitmap_assign_M_template.c"

        // the bitmap of C now contains:
        //  Cb (i,j) = 0:   cij not present, mij zero
        //  Cb (i,j) = 1:   cij present, mij zero
        //  Cb (i,j) = 2:   cij not present, mij 1
        //  Cb (i,j) = 3:   cij present, mij 1

        if (A == NULL)
        { 

            //------------------------------------------------------------------
            // scalar assignment: C<M>(I,J) += scalar
            //------------------------------------------------------------------

            ASSERT (assign_kind == GB_ASSIGN) ;
            // for all entries in IxJ
            #undef  GB_IXJ_WORK
            #define GB_IXJ_WORK(pC)             \
            {                                   \
                int8_t cb = Cb [pC] ;           \
                if (cb == 2)                    \
                {                               \
                    /* Cx [pC] = scalar */      \
                    GB_ASSIGN_SCALAR (pC) ;     \
                    Cb [pC] = 3 ;               \
                    cnvals++ ;                  \
                }                               \
                else if (cb == 3)               \
                {                               \
                    /* Cx [pC] += scalar */     \
                    GB_ACCUM_SCALAR (pC) ;      \
                }                               \
            }
            #include "GB_bitmap_assign_IxJ_template.c"

        }
        else
        {

            //------------------------------------------------------------------
            // matrix assignment: C<M>(I,J) += A or C(I,J)<M> += A
            //------------------------------------------------------------------

            //  for all entries aij in A (A hyper, sparse, bitmap, or full)
            //      if Cb(p) == 0       // do nothing
            //      if Cb(p) == 1       // do nothing
            //      if Cb(p) == 2:
            //          Cx(p) = aij
            //          Cb(p) = 3       // C(iC,jC) is now present, insert
            //          cnvals++
            //      if Cb(p) == 3:
            //          Cx(p) += aij    // C(iC,jC) still present, updated
            //          Cb(p) still 3

            #define GB_AIJ_WORK(pC,pA)          \
            {                                   \
                int8_t cb = Cb [pC] ;           \
                if (cb == 2)                    \
                {                               \
                    /* Cx [pC] = Ax [pA] */     \
                    GB_ASSIGN_AIJ (pC, pA) ;    \
                    Cb [pC] = 3 ;               \
                    cnvals++ ;                  \
                }                               \
                else if (cb == 3)               \
                {                               \
                    /* Cx [pC] += Ax [pA] */    \
                    GB_ACCUM_AIJ (pC, pA) ;     \
                }                               \
            }
            #include "GB_bitmap_assign_A_template.c"
        }

        //----------------------------------------------------------------------
        // final pass: clear M from C or handle C_replace
        //----------------------------------------------------------------------

        if (C_replace)
        {
            // scan all of C for the C_replace phase
            // for row assign: for all entries in C(i,:)
            // for col assign: for all entries in C(:,j)
            // for assign: for all entries in C(:,:)
            // for subassign: for all entries in C(I,J)
                    // 0 -> 0
                    // 1 -> 0  delete this entry
                    // 2 -> 0
                    // 3 -> 1: keep this entry.  already counted above
            #define GB_CIJ_WORK(mij,pC)             \
            {                                       \
                int8_t cb = Cb [pC] ;               \
                cnvals -= (cb == 1) ;               \
                Cb [pC] = (cb == 3) ;               \
            }
            #include "GB_bitmap_assign_C_template.c"
        }
        else
        {
            // clear M from C
            #undef  GB_MASK_WORK
            #define GB_MASK_WORK(pC) Cb [pC] -= 2
            #include "GB_bitmap_assign_M_template.c"
        }
    }

    //--------------------------------------------------------------------------
    // free workspace and return result
    //--------------------------------------------------------------------------

    C->nvals = cnvals ;
    GB_FREE_ALL ;
    ASSERT_MATRIX_OK (C, "final C for bitmap assign, M, accum", GB0) ;
    return (GrB_SUCCESS) ;
}
