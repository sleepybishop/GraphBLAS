//------------------------------------------------------------------------------
// GB_bitmap_assign_fullM_noaccum:  assign to C bitmap, M is bitmap or full
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------
// C<M>(I,J) = A       assign
// C(I,J)<M> = A       subassign

// C<M,repl>(I,J) = A       assign
// C(I,J)<M,repl> = A       subassign

// C<!M>(I,J) = A       assign
// C(I,J)<!M> = A       subassign

// C<!M,repl>(I,J) = A       assign
// C(I,J)<!M,repl> = A       subassign
//------------------------------------------------------------------------------

// C:           bitmap
// M:           present, bitmap or full (not hypersparse or sparse)
// Mask_comp:   true or false
// Mask_struct: true or false
// C_replace:   true or false
// accum:       not present
// A:           matrix (hyper, sparse, bitmap, or full), or scalar
// kind:        assign, row assign, col assign, or subassign

#include "GB_bitmap_assign_methods.h"

#define GB_FREE_ALL ;

GrB_Info GB_bitmap_assign_fullM_noaccum
(
    // input/output:
    GrB_Matrix C,               // input/output matrix in bitmap format
    const bool C_replace,       // descriptor for C
    // inputs:
    const GrB_Index *I,         // I index list
    const int64_t nI,
    const int Ikind,
    const int64_t Icolon [3],
    const GrB_Index *J,         // J index list
    const int64_t nJ,
    const int Jkind,
    const int64_t Jcolon [3],
    const GrB_Matrix M,         // mask matrix, which is present here
    const bool Mask_comp,       // true for !M, false for M
    const bool Mask_struct,     // true if M is structural, false if valued
//  const GrB_BinaryOp accum,   // not present
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

    GBURBLE_BITMAP_ASSIGN ("bit2", M, Mask_comp, NULL) ;
    ASSERT (GB_IS_BITMAP (C)) ;
    ASSERT (GB_IS_BITMAP (M) || GB_IS_FULL (M)) ;
    ASSERT_MATRIX_OK (C, "C for bitmap assign, M full, noaccum", GB0) ;
    ASSERT_MATRIX_OK (M, "M for bitmap assign, M full, noaccum", GB0) ;
    ASSERT_MATRIX_OK_OR_NULL (A, "A for bitmap assign, M full, noaccum", GB0) ;

    //--------------------------------------------------------------------------
    // get inputs
    //--------------------------------------------------------------------------

    GB_GET_C
    GB_GET_M
    GB_GET_A

    //--------------------------------------------------------------------------
    // C_replace phase
    //--------------------------------------------------------------------------

    if (C_replace)
    {
        // for row assign: set Cb(i,:) to zero
        // for col assign: set Cb(:,j) to zero
        // for assign: set all Cb to zero
        // for subassign set all Cb(I,J) to zero
        #define GB_CIJ_WORK(mij,pC)     \
        {                               \
            int8_t cb = Cb [pC] ;       \
            cnvals -= (cb == 1) ;       \
            Cb [pC] = 0 ;               \
        }
        #include "GB_bitmap_assign_C_template.c"
    }

    //--------------------------------------------------------------------------
    // to get the effective value of the mask entry mij
    //--------------------------------------------------------------------------

    #define GB_GET_MIJ(mij,pM)                                  \
        bool mij = GBB (Mb, pM) && GB_mcast (Mx, pM, msize) ;   \
        if (Mask_comp) mij = !mij ;

    //--------------------------------------------------------------------------
    // do the assignment
    //--------------------------------------------------------------------------

    if (A == NULL)
    {

        //----------------------------------------------------------------------
        // scalar assignment: C<M or !M>(I,J) = scalar
        //----------------------------------------------------------------------

        // for all entries in IxJ
        #undef  GB_IXJ_WORK
        #define GB_IXJ_WORK(pC)                 \
        {                                       \
            int64_t pM = GB_GET_pM ;            \
            GB_GET_MIJ (mij, pM) ;              \
            if (mij)                            \
            {                                   \
                int8_t cb = Cb [pC] ;           \
                cnvals += (cb == 0) ;           \
                Cb [pC] = 1 ;                   \
                /* Cx [pC] = scalar */          \
                GB_ASSIGN_SCALAR (pC) ;         \
            }                                   \
        }

        ASSERT (assign_kind == GB_ASSIGN || assign_kind == GB_SUBASSIGN) ;

        switch (assign_kind)
        {
            case GB_ASSIGN :
                #define GB_GET_pM pC
                #include "GB_bitmap_assign_IxJ_template.c"
                break ;
            case GB_SUBASSIGN :
                #define GB_GET_pM (iA + jA * nI)
                #include "GB_bitmap_assign_IxJ_template.c"
                break ;
            default:;
        }

    }
    else
    {

        //----------------------------------------------------------------------
        // matrix assignment: C<M or !M>(I,J) = A
        //----------------------------------------------------------------------

        //----------------------------------------------------------------------
        // assign A into C
        //----------------------------------------------------------------------

        int8_t keep = C_replace ? 1 : 4 ;

        //  for all entries aij in A (A can be hyper, sparse, bitmap, or full)
        //      get the effective value of the mask:
        //          for row assign: get mij = m(jC,0)
        //          for col assign: get mij = m(iC,0)
        //          for assign: get mij = M(iC,jC)
        //          for subassign: get mij = M(i,j)
        //          if complemented: mij = !mij
        //      if mij == 1:
        //          Cx(p) = aij     // C(iC,jC) inserted or updated
        //          Cb(p) = keep

        #define GB_AIJ_WORK(pC,pA)              \
        {                                       \
            int64_t pM = GB_GET_pM ;            \
            GB_GET_MIJ (mij, pM) ;              \
            if (mij)                            \
            {                                   \
                int8_t cb = Cb [pC] ;           \
                cnvals += (cb == 0) ;           \
                Cb [pC] = keep ;                \
                /* Cx [pC] = Ax [pA] */         \
                GB_ASSIGN_AIJ (pC, pA) ;        \
            }                                   \
        }

        switch (assign_kind)
        {
            case GB_ROW_ASSIGN :
                #define GB_GET_pM jC
                #include "GB_bitmap_assign_A_template.c"
                break ;
            case GB_COL_ASSIGN :
                #define GB_GET_pM iC
                #include "GB_bitmap_assign_A_template.c"
                break ;
            case GB_ASSIGN :
                #define GB_GET_pM pC
                #include "GB_bitmap_assign_A_template.c"
                break ;
            case GB_SUBASSIGN :
                #define GB_GET_pM (iA + jA * nI)
                #include "GB_bitmap_assign_A_template.c"
                break ;
            default:;
        }

        //----------------------------------------------------------------------
        // clear entries from C that were not in A
        //----------------------------------------------------------------------

        if (!C_replace)
        {
            // for row assign: for all entries in C(i,:)
            // for col assign: for all entries in C(:,j)
            // for assign: for all entries in C(:,:)
            // for subassign: for all entries in C(I,J)
                // get the effective value of the mask
                // if mij == 1
                    // 0 -> 0
                    // 1 -> 0           delete because aij not present
                    // keep -> 1

            #define GB_CIJ_WORK(mij,pC)         \
            {                                   \
                if (mij)                        \
                {                               \
                    int8_t cb = Cb [pC] ;       \
                    cnvals -= (cb == 1) ;       \
                    Cb [pC] = (cb > 1) ;        \
                }                               \
            }
            #include "GB_bitmap_assign_C_template.c"
        }
    }

    //--------------------------------------------------------------------------
    // return result
    //--------------------------------------------------------------------------

    C->nvals = cnvals ;
    ASSERT_MATRIX_OK (C, "final C for bitmap assign, M full, noaccum", GB0) ;
    return (GrB_SUCCESS) ;
}
