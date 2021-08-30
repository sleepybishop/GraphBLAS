//------------------------------------------------------------------------------
// gbdeserialize: deserialize a blob into a matrix
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: GPL-3.0-or-later

//------------------------------------------------------------------------------

// gbdeserialize is an interface to GxB_Matrix_deserialize.

// Usage:

// A = gbdeserialize (blob)

#include "gb_interface.h"

#define USAGE "usage: A = GrB.deserialize (blob)"

void mexFunction
(
    int nargout,
    mxArray *pargout [ ],
    int nargin,
    const mxArray *pargin [ ]
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    gb_usage (nargin == 1 && nargout <= 1, USAGE) ;
    CHECK_ERROR (mxGetClassID (pargin [0]) != mxUINT8_CLASS
        || mxGetN (pargin [0]) != 1, "blob must be uint8 column vector") ;
    void *blob = mxGetData (pargin [0]) ;
    size_t blob_size = mxGetM (pargin [0]) ;

    //--------------------------------------------------------------------------
    // deserialize the blob into a matrix
    //--------------------------------------------------------------------------

    GrB_Matrix C = NULL ;
    OK (GxB_Matrix_deserialize (&C, blob, blob_size, NULL, NULL)) ;

    //--------------------------------------------------------------------------
    // export the output matrix C
    //--------------------------------------------------------------------------

    pargout [0] = gb_export (&C, KIND_GRB) ;
    GB_WRAPUP ;
}
