//------------------------------------------------------------------------------
// GxB_IndexUnaryOp_ttype_name: return the type_name of thunk for z=f(x,thunk)
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"

GrB_Info GxB_IndexUnaryOp_ttype_name    // return the name of the type of thunk
(
    char *type_name,        // name of the type (char array of size at least
                            // GxB_MAX_NAME_LEN, owned by the user application).
    const GrB_IndexUnaryOp op
)
{ 

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_IndexUnaryOp_ttype_name (type_name, op)") ;
    GB_RETURN_IF_NULL (type_name) ;
    GB_RETURN_IF_NULL_OR_FAULTY (op) ;
    ASSERT_INDEXUNARYOP_OK (op, "op for ttype_name", GB0) ;

    //--------------------------------------------------------------------------
    // get the type_name
    //--------------------------------------------------------------------------

    memcpy (type_name, op->ytype->name, GxB_MAX_NAME_LEN) ;
    return (GrB_SUCCESS) ;
}
