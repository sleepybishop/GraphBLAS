//------------------------------------------------------------------------------
// GB_emult.h: definitions for GB_emult
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#ifndef GB_EMULT_H
#define GB_EMULT_H
#include "GB.h"
#include "GB_bitmap_assign_methods.h"

#define GB_EMULT_METHOD_99 99       /* punt */

#define GB_EMULT_METHOD_ADD 0       /* use GB_add instead of emult */

#define GB_EMULT_METHOD_01A 1       /* use GB_emult_01 (A,B) */
#define GB_EMULT_METHOD_01B (-1)    /* use GB_emult_01 (B,A, flipxy true) */

#define GB_EMULT_METHOD_18  18      /* use bitmap method 18 */
#define GB_EMULT_METHOD_19  19      /* use bitmap method 19 */
#define GB_EMULT_METHOD_20  20      /* use bitmap method 20 */

#define GB_EMULT_METHOD_100  100

#define GB_EMULT_METHOD_101A 101    /* use GB_emult_101 (M,A,B) */
#define GB_EMULT_METHOD_101B (-101) /* use GB_emult_101 (M,B,A, flipxy true) */

GrB_Info GB_emult           // C=A.*B or C<M>=A.*B
(
    GrB_Matrix *Chandle,    // output matrix (unallocated on input)
    const GrB_Type ctype,   // type of output matrix C
    const bool C_is_csc,    // format of output matrix C
    const GrB_Matrix M,     // optional mask, unused if NULL.  Not complemented
    const bool Mask_struct, // if true, use the only structure of M
    const bool Mask_comp,   // if true, use the !M
    bool *mask_applied,
    const GrB_Matrix A,     // input A matrix
    const GrB_Matrix B,     // input B matrix
    const GrB_BinaryOp op,  // op to perform C = op (A,B)
    GB_Context Context
) ;

GrB_Info GB_emult_phase0        // find vectors in C for C=A.*B or C<M>=A.*B
(
    int64_t *p_Cnvec,           // # of vectors to compute in C
    const int64_t *GB_RESTRICT *Ch_handle,  // Ch is M->h, A->h, B->h, or NULL
    int64_t *GB_RESTRICT *C_to_M_handle,    // C_to_M: size Cnvec, or NULL
    int64_t *GB_RESTRICT *C_to_A_handle,    // C_to_A: size Cnvec, or NULL
    int64_t *GB_RESTRICT *C_to_B_handle,    // C_to_B: size Cnvec, or NULL
    int *C_sparsity,            // sparsity structure of C
    // original input:
    const GrB_Matrix M,         // optional mask, may be NULL
    const GrB_Matrix A,
    const GrB_Matrix B,
    GB_Context Context
) ;

GrB_Info GB_emult_phase1                // count nnz in each C(:,j)
(
    int64_t *GB_RESTRICT *Cp_handle,        // output of size Cnvec+1
    int64_t *Cnvec_nonempty,                // # of non-empty vectors in C
    // tasks from phase1a:
    GB_task_struct *GB_RESTRICT TaskList,   // array of structs
    const int C_ntasks,                       // # of tasks
    const int C_nthreads,                     // # of threads to use
    // analysis from phase0:
    const int64_t Cnvec,
    const int64_t *GB_RESTRICT Ch,          // Ch is NULL, or shallow pointer
    const int64_t *GB_RESTRICT C_to_M,
    const int64_t *GB_RESTRICT C_to_A,
    const int64_t *GB_RESTRICT C_to_B,
    // original input:
    const GrB_Matrix M,             // optional mask, may be NULL
    const bool Mask_struct,         // if true, use the only structure of M
    const bool Mask_comp,           // if true, use !M
    const GrB_Matrix A,
    const GrB_Matrix B,
    GB_Context Context
) ;

GrB_Info GB_emult_phase2                // C=A.*B or C<M>=A.*B
(
    GrB_Matrix *Chandle,    // output matrix (unallocated on input)
    const GrB_Type ctype,   // type of output matrix C
    const bool C_is_csc,    // format of output matrix C
    const GrB_BinaryOp op,  // op to perform C = op (A,B)
    // from phase1:
    const int64_t *GB_RESTRICT Cp,      // vector pointers for C
    const int64_t Cnvec_nonempty,       // # of non-empty vectors in C
    // tasks from phase1a:
    const GB_task_struct *GB_RESTRICT TaskList,  // array of structs
    const int C_ntasks,                         // # of tasks
    const int C_nthreads,                       // # of threads to use
    // analysis from phase0:
    const int64_t Cnvec,
    const int64_t *GB_RESTRICT Ch,
    const int64_t *GB_RESTRICT C_to_M,
    const int64_t *GB_RESTRICT C_to_A,
    const int64_t *GB_RESTRICT C_to_B,
    const int C_sparsity,
    // from GB_emult_sparsity:
    const int emult_method,
    // original input:
    const GrB_Matrix M,             // optional mask, may be NULL
    const bool Mask_struct,         // if true, use the only structure of M
    const bool Mask_comp,           // if true, use !M
    const GrB_Matrix A,
    const GrB_Matrix B,
    GB_Context Context
) ;

int GB_emult_sparsity       // return the sparsity structure for C
(
    // output:
    bool *apply_mask,       // if true then mask will be applied by GB_emult
    int *emult_method,      // method to use (0: add, 1: GB_emult_01, etc)
    // input:
    const GrB_Matrix M,     // optional mask for C, unused if NULL
    const bool Mask_comp,   // if true, use !M
    const GrB_Matrix A,     // input A matrix
    const GrB_Matrix B      // input B matrix
) ;

GrB_Info GB_emult_01        // C=A.*B when A is sparse/hyper, B bitmap/full
(
    GrB_Matrix *Chandle,    // output matrix (unallocated on input)
    const GrB_Type ctype,   // type of output matrix C
    const bool C_is_csc,    // format of output matrix C
    const GrB_Matrix A,     // input A matrix (sparse/hyper)
    const GrB_Matrix B,     // input B matrix (bitmap/full)
    GrB_BinaryOp op,        // op to perform C = op (A,B)
    bool flipxy,            // if true use fmult(y,x) else fmult(x,y)
    GB_Context Context
) ;

#endif

