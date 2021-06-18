//------------------------------------------------------------------------------
// GB_AxB_defs__any_pair_iso: definitions for a single semiring
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// If this file is in the Generated/ folder, do not edit it (auto-generated).

#include "GB_dev.h"

#if 1
#include "GB.h"
#include "GB_control.h"
#include "GB_bracket.h"
#include "GB_sort.h"
#include "GB_atomics.h"
#include "GB_AxB_saxpy.h"
#include "GB_AxB__include.h"
#include "GB_unused.h"
#include "GB_bitmap_assign_methods.h"
#include "GB_ek_slice_search.c"

// This C=A*B semiring is defined by the following types and operators:

// A'*B (dot2):        GB (_Adot2B__any_pair_iso)
// A'*B (dot3):        GB (_Adot3B__any_pair_iso)
// C+=A'*B (dot4):     GB (_Adot4B__any_pair_iso)
// A*B (saxpy3):       GB (_Asaxpy3B__any_pair_iso)
//     no mask:        GB (_Asaxpy3B_noM__any_pair_iso)
//     mask M:         GB (_Asaxpy3B_M__any_pair_iso)
//     mask !M:        GB (_Asaxpy3B_notM__any_pair_iso)
// A*B (saxpy bitmap): GB (_AsaxbitB__any_pair_iso)

// C type:   iso
// A type:   any type
// B type:   any type

// Multiply: 
// Add:      ;
//           'any' monoid?  1
//           atomic?        1
//           OpenMP atomic? 1
// MultAdd:  
// Identity:  
// Terminal: break ;

#define GB_ATYPE \
    any type

#define GB_BTYPE \
    any type

#define GB_CTYPE \
    iso

#define GB_ASIZE \
    0

#define GB_BSIZE \
    0 

#define GB_CSIZE \
    0

// true for int64, uint64, float, double, float complex, and double complex 
#define GB_CTYPE_IGNORE_OVERFLOW \
    1

// aik = Ax [pA]
#define GB_GETA(aik,Ax,pA,A_iso) \
    ;

// bkj = Bx [pB]
#define GB_GETB(bkj,Bx,pB,B_iso) \
    ;

// Gx [pG] = Ax [pA]
#define GB_LOADA(Gx,pG,Ax,pA,A_iso) \
    ;

// Gx [pG] = Bx [pB]
#define GB_LOADB(Gx,pG,Bx,pB,B_iso) \
    ;

#define GB_CX(p) \
    

// multiply operator
#define GB_MULT(z, x, y, i, k, j) \
    

// cast from a real scalar (or 2, if C is complex) to the type of C
#define GB_CTYPE_CAST(x,y) \
    

// cast from a real scalar (or 2, if A is complex) to the type of A
#define GB_ATYPE_CAST(x,y) \
    

// multiply-add
#define GB_MULTADD(z, x, y, i, k, j) \
    

// monoid identity value
#define GB_IDENTITY \
     

// 1 if the identity value can be assigned via memset, with all bytes the same
#define GB_HAS_IDENTITY_BYTE \
    0

// identity byte, for memset
#define GB_IDENTITY_BYTE \
    (none)

// break if cij reaches the terminal value (dot product only)
#define GB_DOT_TERMINAL(cij) \
    break ;

// simd pragma for dot-product loop vectorization
#define GB_PRAGMA_SIMD_DOT(cij) \
    ;

// simd pragma for other loop vectorization
#define GB_PRAGMA_SIMD_VECTORIZE GB_PRAGMA_SIMD

// 1 for the PLUS_PAIR_(real) semirings, not for the complex case
#define GB_IS_PLUS_PAIR_REAL_SEMIRING \
    0

// declare the cij scalar (initialize cij to zero for PLUS_PAIR)
#define GB_CIJ_DECLARE(cij) \
    

// cij = Cx [pC] for dot4 method only
#define GB_GET4C(cij,p) \
    

// Cx [pC] = cij
#define GB_PUTC(cij,p) \
    

// Cx [p] = t
#define GB_CIJ_WRITE(p,t) \
    

// C(i,j) += t
#define GB_CIJ_UPDATE(p,t) \
    ;

// x + y
#define GB_ADD_FUNCTION(x,y) \
    

// bit pattern for bool, 8-bit, 16-bit, and 32-bit integers
#define GB_CTYPE_BITS \
    0

// 1 if monoid update can skipped entirely (the ANY monoid)
#define GB_IS_ANY_MONOID \
    1

// 1 if monoid update is EQ
#define GB_IS_EQ_MONOID \
    0

// 1 if monoid update can be done atomically, 0 otherwise
#define GB_HAS_ATOMIC \
    1

// 1 if monoid update can be done with an OpenMP atomic update, 0 otherwise
#if GB_MICROSOFT
    #define GB_HAS_OMP_ATOMIC \
        0
#else
    #define GB_HAS_OMP_ATOMIC \
        1
#endif

// 1 for the ANY_PAIR_ISO semiring
#define GB_IS_ANY_PAIR_SEMIRING \
    1

// 1 if PAIR is the multiply operator 
#define GB_IS_PAIR_MULTIPLIER \
    1

// 1 if monoid is PLUS_FC32
#define GB_IS_PLUS_FC32_MONOID \
    0

// 1 if monoid is PLUS_FC64
#define GB_IS_PLUS_FC64_MONOID \
    0

// 1 if monoid is ANY_FC32
#define GB_IS_ANY_FC32_MONOID \
    0

// 1 if monoid is ANY_FC64
#define GB_IS_ANY_FC64_MONOID \
    0

// 1 if monoid is MIN for signed or unsigned integers
#define GB_IS_IMIN_MONOID \
    0

// 1 if monoid is MAX for signed or unsigned integers
#define GB_IS_IMAX_MONOID \
    0

// 1 if monoid is MIN for float or double
#define GB_IS_FMIN_MONOID \
    0

// 1 if monoid is MAX for float or double
#define GB_IS_FMAX_MONOID \
    0

// 1 for the FIRSTI or FIRSTI1 multiply operator
#define GB_IS_FIRSTI_MULTIPLIER \
    0

// 1 for the FIRSTJ or FIRSTJ1 multiply operator
#define GB_IS_FIRSTJ_MULTIPLIER \
    0

// 1 for the SECONDJ or SECONDJ1 multiply operator
#define GB_IS_SECONDJ_MULTIPLIER \
    0

// atomic compare-exchange
#define GB_ATOMIC_COMPARE_EXCHANGE(target, expected, desired) \
    ;

// Hx [i] = t
#define GB_HX_WRITE(i,t) \
    ;

// Cx [p] = Hx [i]
#define GB_CIJ_GATHER(p,i) \
    ;

// Cx [p] += Hx [i]
#define GB_CIJ_GATHER_UPDATE(p,i) \
    ;

// Hx [i] += t
#define GB_HX_UPDATE(i,t) \
    ;

// memcpy (&(Cx [p]), &(Hx [i]), len)
#define GB_CIJ_MEMCPY(p,i,len) \
    ;

// 1 if the semiring has a concise bitmap multiply-add
#define GB_HAS_BITMAP_MULTADD \
    1

// concise statement(s) for the bitmap case:
//  if (exists)
//      if (cb == 0)
//          cx = ax * bx
//          cb = 1
//      else
//          cx += ax * bx
#define GB_BITMAP_MULTADD(cb,cx,exists,ax,bx) \
    cb |= exists

// define X for bitmap multiply-add
#define GB_XINIT \
    ;

// load X [1] = bkj for bitmap multiply-add
#define GB_XLOAD(bkj) \
    ;

// disable this semiring and use the generic case if these conditions hold
#define GB_DISABLE \
    0

#endif
