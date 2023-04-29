//------------------------------------------------------------------------------
// GxB_Context_get: get a field of a Context
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2023, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

#include "GB.h"

//------------------------------------------------------------------------------
// GxB_Context_get_INT32:  get a Context option (int32_t)
//------------------------------------------------------------------------------

GrB_Info GxB_Context_get_INT32      // get a parameter of a Context
(
    GxB_Context Context,            // Context to query
    GxB_Context_Field field,        // parameter to query
    int32_t *value                  // return value from the Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_Context_get_INT32 (Context, field, &value)") ;
    GB_RETURN_IF_NULL_OR_FAULTY (Context) ;
    GB_RETURN_IF_NULL (value) ;

    //--------------------------------------------------------------------------
    // get the parameter
    //--------------------------------------------------------------------------

    switch (field)
    {

        case GxB_CONTEXT_NTHREADS :         // same as GxB_NTHREADS
GB_GOTCHA ;

            (*value) = GB_Context_nthreads_max_get (Context) ;
            break ;

        case GxB_CONTEXT_GPU_ID :           // same as GxB_GPU_ID
GB_GOTCHA ;

            (*value) = GB_Context_gpu_id_get (Context) ;
            break ;

        default : 
GB_GOTCHA ;

            return (GrB_INVALID_VALUE) ;
    }

    #pragma omp flush
    return (GrB_SUCCESS) ;
}

//------------------------------------------------------------------------------
// GxB_Context_get_FP64: get a Context option (double scalar)
//------------------------------------------------------------------------------

GrB_Info GxB_Context_get_FP64       // get a parameter in a Context
(
    GxB_Context Context,            // Context to query
    GxB_Context_Field field,        // parameter to query
    double *value                   // return value from the Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_Context_get_FP64 (Context, field, &value)") ;
    GB_RETURN_IF_NULL_OR_FAULTY (Context) ;
    GB_RETURN_IF_NULL (value) ;

    //--------------------------------------------------------------------------
    // get the parameter
    //--------------------------------------------------------------------------

    switch (field)
    {

        case GxB_CONTEXT_CHUNK :         // same as GxB_CHUNK
GB_GOTCHA ;

            (*value) = GB_Context_chunk_get (Context) ;
            break ;

        default : 
GB_GOTCHA ;

            return (GrB_INVALID_VALUE) ;
    }

    #pragma omp flush
    return (GrB_SUCCESS) ;
}

//------------------------------------------------------------------------------
// GxB_Context_get: get a Context option (va_arg variant)
//------------------------------------------------------------------------------

GrB_Info GxB_Context_get            // get a parameter in a Context
(
    GxB_Context Context,            // Context to query
    GxB_Context_Field field,        // parameter to query
    ...                             // return value of the descriptor
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    GB_WHERE1 ("GxB_Context_get (desc, field, &value)") ;
    GB_RETURN_IF_NULL_OR_FAULTY (Context) ;

    //--------------------------------------------------------------------------
    // get the parameter
    //--------------------------------------------------------------------------

    va_list ap ;

    switch (field)
    {

        case GxB_CONTEXT_NTHREADS :         // same as GxB_NTHREADS
GB_GOTCHA ;

            {
                va_start (ap, field) ;
                int *value = va_arg (ap, int *) ;
                va_end (ap) ;
                GB_RETURN_IF_NULL (value) ;
                (*value) = GB_Context_nthreads_max_get (Context) ;
            }
            break ;

        case GxB_CONTEXT_GPU_ID :           // same as GxB_GPU_ID
GB_GOTCHA ;

            {
                va_start (ap, field) ;
                int *value = va_arg (ap, int *) ;
                va_end (ap) ;
                GB_RETURN_IF_NULL (value) ;
                (*value) = GB_Context_gpu_id_get (Context) ;
            }
            break ;

        case GxB_CONTEXT_CHUNK :            // same as GxB_CHUNK
GB_GOTCHA ;

            {
                va_start (ap, field) ;
                double *value = va_arg (ap, double *) ;
                va_end (ap) ;
                GB_RETURN_IF_NULL (value) ;
                (*value) = GB_Context_chunk_get (Context) ;
            }
            break ;

        default : 
GB_GOTCHA ;

            return (GrB_INVALID_VALUE) ;
    }

    #pragma omp flush
    return (GrB_SUCCESS) ;
}

