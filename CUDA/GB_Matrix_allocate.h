
#ifndef GB_MATRIX_ALLOCATE_H
#define GB_MATRIX_ALLOCATE_H
#include "matrix.h"
#include "pmr_malloc.h"

#ifdef __cplusplus
extern "C"
#endif
{

    GrB_Matrix GB_matrix_allocate
    (   
        GrB_Type type,
        int64_t nrows,
        int64_t ncols,
        int sparsity,   //GxB_FULL, ..
        bool is_csc,
        bool iso,
        int64_t anz,    // ignored if sparsity is GxB_FULL or GxB_BITMAP
        int64_t nvec    // hypersparse only
    ) ;

    GrB_Vector GB_Vector_allocate
    (   
        GrB_Type type,
        int64_t length,
        int sparsity,   // GxB_FULL, GxB_BITMAP, or GxB_SPARSE
        bool iso,
        int64_t anz     // ignored if sparsity is GxB_FULL or GxB_BITMAP
    ) ;

    GxB_Scalar /* FIXME for master: GrB_Scalar*/ GB_Scalar_allocate
    (   
        GrB_Type type,
        int sparsity,   // GxB_FULL, GxB_BITMAP, or GxB_SPARSE
    ) ;
}

#endif

