//------------------------------------------------------------------------------
// GB_reduce_to_vector: reduce a matrix to a vector using a binary op
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
// http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

//------------------------------------------------------------------------------

// CALLS:     GB_build

// C<M> = accum (C,reduce(A)) where C is n-by-1.  Reduces a matrix A or A'
// to a vector.

#include "GB.h"
#ifndef GBCOMPACT
#include "GB_red__include.h"
#endif

#define GB_FREE_ALL GB_MATRIX_FREE (&T) ;

GrB_Info GB_reduce_to_vector        // C<M> = accum (C,reduce(A))
(
    GrB_Matrix C,                   // input/output for results, size n-by-1
    const GrB_Matrix M,             // optional M for C, unused if NULL
    const GrB_BinaryOp accum,       // optional accum for z=accum(C,T)
    const GrB_BinaryOp reduce,      // reduce operator for T=reduce(A)
    const GB_void *terminal,        // for early exit (NULL if none)
    const GrB_Matrix A,             // first input:  matrix A
    const GrB_Descriptor desc,      // descriptor for C, M, and A
    GB_Context Context
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (GB_ALIAS_OK2 (C, M, A)) ;

    GB_RETURN_IF_NULL_OR_FAULTY (C) ;
    GB_RETURN_IF_FAULTY (M) ;
    GB_RETURN_IF_FAULTY (accum) ;
    GB_RETURN_IF_NULL_OR_FAULTY (A) ;
    GB_RETURN_IF_FAULTY (desc) ;

    ASSERT_OK (GB_check (C, "C input for reduce_BinaryOp", GB0)) ;
    ASSERT_OK_OR_NULL (GB_check (M, "M for reduce_BinaryOp", GB0)) ;
    ASSERT_OK_OR_NULL (GB_check (accum, "accum for reduce_BinaryOp", GB0)) ;
    ASSERT_OK (GB_check (reduce, "reduce for reduce_BinaryOp", GB0)) ;
    ASSERT_OK (GB_check (A, "A input for reduce_BinaryOp", GB0)) ;
    ASSERT_OK_OR_NULL (GB_check (desc, "desc for reduce_BinaryOp", GB0)) ;

    GrB_Matrix T = NULL ;

    // get the descriptor
    GB_GET_DESCRIPTOR (info, desc, C_replace, Mask_comp, A_transpose, xx1, xx2);

    // C and M are n-by-1 GrB_Vector objects, typecasted to GrB_Matrix
    ASSERT (GB_VECTOR_OK (C)) ;
    ASSERT (GB_IMPLIES (M != NULL, GB_VECTOR_OK (M))) ;

    // check domains and dimensions for C<M> = accum (C,T)
    GrB_Type ttype = reduce->ztype ;
    GB_OK (GB_compatible (C->type, C, M, accum, ttype, Context)) ;

    // check types of reduce
    if (reduce->xtype != reduce->ztype || reduce->ytype != reduce->ztype)
    { 
        // all 3 types of z = reduce (x,y) must be the same.  reduce must also
        // be associative but there is no way to check this in general.
        return (GB_ERROR (GrB_DOMAIN_MISMATCH, (GB_LOG,
            "All domains of reduction operator must be identical;\n"
            "operator is: [%s] = %s ([%s],[%s])", reduce->ztype->name,
            reduce->name, reduce->xtype->name, reduce->ytype->name))) ;
    }

    // T = reduce (T,A) must be compatible
    if (!GB_Type_compatible (A->type, reduce->ztype))
    { 
        return (GB_ERROR (GrB_DOMAIN_MISMATCH, (GB_LOG,
            "incompatible type for reduction operator z=%s(x,y):\n"
            "input matrix A of type [%s]\n"
            "cannot be typecast to reduction operator of type [%s]",
            reduce->name, A->type->name, reduce->ztype->name))) ;
    }

    // check the dimensions
    int64_t n = GB_NROWS (C) ;
    if (A_transpose)
    {
        if (n != GB_NCOLS (A))
        { 
            return (GB_ERROR (GrB_DIMENSION_MISMATCH, (GB_LOG,
                "w=reduce(A'):  length of w is "GBd";\n"
                "it must match the number of columns of A, which is "GBd".",
                n, GB_NCOLS (A)))) ;
        }
    }
    else
    {
        if (n != GB_NROWS(A))
        { 
            return (GB_ERROR (GrB_DIMENSION_MISMATCH, (GB_LOG,
                "w=reduce(A):  length of w is "GBd";\n"
                "it must match the number of rows of A, which is "GBd".",
                n, GB_NROWS (A)))) ;
        }
    }

    // quick return if an empty mask is complemented
    GB_RETURN_IF_QUICK_MASK (C, C_replace, M, Mask_comp) ;

    //--------------------------------------------------------------------------
    // delete any lingering zombies and assemble any pending tuples
    //--------------------------------------------------------------------------

    // GB_WAIT (C) ;
    GB_WAIT (M) ;
    GB_WAIT (A) ;

    ASSERT (!GB_PENDING (A)) ; ASSERT (!GB_ZOMBIES (A)) ;

    //--------------------------------------------------------------------------
    // handle the CSR/CSC format of A
    //--------------------------------------------------------------------------

    // the result vector T is in CSC format
    if (!(A->is_csc))
    { 
        A_transpose = !A_transpose ;
    }

    //--------------------------------------------------------------------------
    // T = reduce (A) or reduce (A')
    //--------------------------------------------------------------------------

    // T is created below so that it can be typecasted to a GrB_Vector when
    // done: non-hypersparse n-by-1 matrix in CSC format.

    // T = reduce_to_vector (A) or reduce_to_vector (A'), which is T = sum (A')
    // or sum (A), in MATLAB notation, except where where 'sum' is any
    // associative operator.

    // By default, T(i) = op (A (i,:)) is a vector whose length is the same as
    // the number of rows of A.  T(i) is the reduction of all entries in the
    // ith row of A.  If A_transpose is true, the T is computed as if A were
    // transposed first, and thus its length is equal to the number of vectors
    // of the input matrix A.  The use of A_transpose is the opposite of
    // MATLAB, since sum(A) in MATLAB sums up the columns of A, and sum(A')
    // sums up the rows of A..

    // T is an n-by-1 GrB_Matrix that represents the vector.  It is computed
    // as a GrB_Matrix so it can be passed to GB_accum_mask without
    // typecasting.

    ASSERT (n == (A_transpose) ? A->vdim : A->vlen) ;

    //--------------------------------------------------------------------------
    // scalar workspace
    //--------------------------------------------------------------------------

    size_t asize = A->type->size ;
    int    acode = A->type->code ;
    const int64_t *restrict Ai = A->i ;
    const GB_void *restrict Ax = A->x ;
    int64_t anvec = A->nvec ;
    int64_t anz = GB_NNZ (A) ;

    size_t zsize = reduce->ztype->size ;
    int    zcode = reduce->ztype->code ;

    //--------------------------------------------------------------------------
    // determine the number of threads to use
    //--------------------------------------------------------------------------

    // TODO find a good chunk size
    // #define GB_CHUNK (1024*1024)
//    #define GB_CHUNK (4*1024)
    #define GB_CHUNK (2)
    GB_GET_NTHREADS (nthreads, Context) ;
    // TODO reduce nthreads for small problem (work: about O(anz))

    nthreads = GB_IMIN (anz / GB_CHUNK, nthreads) ;
    nthreads = GB_IMAX (nthreads, 1) ;

    //--------------------------------------------------------------------------
    // T = reduce(A) or reduce(A')
    //--------------------------------------------------------------------------

    GxB_binary_function freduce = reduce->function ;
    GB_cast_function cast_A_to_Z = GB_cast_factory (zcode, acode) ;
    bool nocasting = (A->type == reduce->ztype) ;

    if (A_transpose)
    {

        //----------------------------------------------------------------------
        // T = reduce(A'), where T(j) = reduce (A (:,j))
        //----------------------------------------------------------------------

        // Each vector A(:,j) is reduced to the scalar T(j)

        //----------------------------------------------------------------------
        // allocate T, including T->p, T->i, and T->x.  T is not hypersparse.
        //----------------------------------------------------------------------

        // since T is a GrB_Vector, it is CSC and not hypersparse
        GB_CREATE (&T, ttype, n, 1, GB_Ap_calloc, true,
            GB_FORCE_NONHYPER, GB_HYPER_DEFAULT, 1, anvec, true, Context) ;
        GB_OK (info) ;
        ASSERT (GB_VECTOR_OK (T)) ;

        T->p [0] = 0 ;
        T->p [1] = anvec ;
        int64_t *restrict Ti = T->i ;
        GB_void *restrict Tx = T->x ;
        T->nvec_nonempty = (anvec > 0) ? 1 : 0 ;
        T->magic = GB_MAGIC ;

        //----------------------------------------------------------------------
        // symbolic phase
        //----------------------------------------------------------------------

        // Construct the pattern of T.  The kth vector in A creates one entry
        // in T, but it is flagged as a zombie if it is empty.

        int64_t nzombies = 0 ;
        const int64_t *restrict Ah = A->h ;
        const int64_t *restrict Ap = A->p ;

        // TODO benchmark this in parallel
        //      #pragma omp parallel for num_threads(nthreads) \\
        //          schedule(static) reduction(+:nzombies)
        for (int64_t k = 0 ; k < anvec ; k++)
        {
            // if A(:,j) is empty, then the entry in T becomes a zombie
            int64_t j = (Ah == NULL) ? k : Ah [k] ;
            int64_t jnz = Ap [k+1] - Ap [k] ;
            if (jnz == 0)
            {
                // A(:,j) is empty: T(j) is a zombie
                Ti [k] = GB_FLIP (j) ;
                nzombies++ ;
            }
            else
            {
                // A(:,j) has at least one entry; T(j) is live
                Ti [k] = j ;
            }
        }

        if (A->nvec_nonempty < 0)
        {
            A->nvec_nonempty = anvec - nzombies ;
        }
        ASSERT (A->nvec_nonempty == (anvec - nzombies)) ;
        T->nzombies = nzombies ;

        //----------------------------------------------------------------------
        // slice the entries of A for the numeric phase
        //----------------------------------------------------------------------

        // TODO consider creating ntasks > nthreads.  Perhaps ntasks = 8 *
        // nthreads or so, for better load-balancing.  A slice whose entries
        // are all in a single vector will be faster than a slice with each
        // entry in a unique vector.  It might be 4x or 8x faster, if SIMD
        // vectorization can be exploited.

        // Thread tid does entries pstart_slice [tid] to pstart_slice [tid+1]-1
        // and vectors kfirst_slice [tid] to klast_slice [tid].  The first and
        // last vectors may be shared with prior slices and subsequent slices.

        int64_t pstart_slice [nthreads+1] ;
        int64_t kfirst_slice [nthreads] ;
        int64_t klast_slice  [nthreads] ;

        GB_ek_slice (pstart_slice, kfirst_slice, klast_slice, A, nthreads) ;

        //----------------------------------------------------------------------
        // numeric phase: launch the switch factory
        //----------------------------------------------------------------------

        bool done = false ;

        #ifndef GBCOMPACT

            #define GB_red(opname,aname) GB_red_eachvec_ ## opname ## aname
            #define GB_RED_WORKER(opname,aname,atype)                       \
            {                                                               \
                GB_red (opname, aname) ((atype *) Tx, A,                    \
                    kfirst_slice, klast_slice, pstart_slice, nthreads) ;    \
                done = true ;                                               \
            }                                                               \
            break ;

            if (nocasting)
            { 
                // controlled by opcode and typecode.  No typecasting is done.
                GB_Opcode opcode = reduce->opcode ;
                GB_Type_code typecode = acode ;
                ASSERT (typecode <= GB_UDT_code) ;
                #include "GB_red_factory.c"
            }

        #endif

        //----------------------------------------------------------------------
        // generic worker: with typecasting
        //----------------------------------------------------------------------

        if (!done)
        {

            #define GB_ATYPE GB_void
            #define GB_CTYPE GB_void

            // workspace for each thread
            #define GB_REDUCTION_WORKSPACE(W, nthreads)             \
                GB_void W [nthreads*zsize]

            // ztype s = (ztype) Ax [p], with typecast
            #define GB_CAST_ARRAY_TO_SCALAR(s,Ax,p)                 \
                GB_void s [zsize] ;                                 \
                cast_A_to_Z (s, Ax +((p)*asize), zsize) ;           \

            // s += (ztype) Ax [p], with typecast
            #define GB_ADD_CAST_ARRAY_TO_SCALAR(s, Ax, p)           \
                GB_void awork [zsize] ;                             \
                cast_A_to_Z (awork, Ax +((p)*asize), zsize) ;       \
                freduce (s, s, awork) ;

            // W [k] = s, no typecast
            #define GB_COPY_SCALAR_TO_ARRAY(W,k,s)                  \
                memcpy (W +((k)*zsize), s, zsize) ;

            // W [k] = S [i], no typecast
            #define GB_COPY_ARRAY_TO_ARRAY(W,k,S,i)                 \
                memcpy (W +((k)*zsize), S +((i)*zsize), zsize) ;

            // W [k] += S [i], no typecast
            #define GB_ADD_ARRAY_TO_ARRAY(W,k,S,i)                  \
                freduce (W +((k)*zsize), W +((k)*zsize), S +((i)*zsize)) ;

            // W [k] += s, no typecast
            #define GB_ADD_SCALAR_TO_ARRAY(W,k,s)                   \
                freduce (W +((k)*zsize), W +((k)*zsize), s) ;

            // break if terminal value reached
            #define GB_BREAK_IF_TERMINAL(t)                         \
                if (terminal != NULL)                               \
                {                                                   \
                    if (memcmp (t, terminal, zsize) == 0) break ;   \
                }

            #include "GB_reduce_each_vector.c"

        }

        //----------------------------------------------------------------------
        // wrapup: delete any zombies
        //----------------------------------------------------------------------

        ASSERT_OK (GB_check (T, "T before wait", GB_FLIP (GB0)));

        if (nzombies > 0)
        {
            // TODO: if GB_accum_mask could tolerate zombies in T on input,
            // this could be skipped
            ASSERT (GB_VECTOR_OK (T)) ;
            ASSERT (!GB_PENDING (T)) ;
            ASSERT (GB_ZOMBIES (T)) ;
            GB_OK (GB_wait (T, Context)) ;
        }

        ASSERT_OK (GB_check (T, "T output = reduce_each_vector (A)", GB0)) ;
    }
    else
    {

        //----------------------------------------------------------------------
        // T = reduce(A), where T(i) = reduce (A (i,:))
        //----------------------------------------------------------------------

        //----------------------------------------------------------------------
        // select the method
        //----------------------------------------------------------------------

        // When A_transpose is false (after flipping it to account for the
        // CSR/CSC format), n is A->vlen, the vector length of A.  This is
        // the number of rows of a CSC matrix, or the # of columns of a CSR
        // matrix.  The matrix A itself requires O(vdim+anz) memory if
        // non-hypersparse and O(anz) if hypersparse.  This does not depend on
        // A->vlen.  So if the vector length is really huge (when anz << n),
        // the bucket method would fail.  Thus, the qsort method, below, is
        // used when A is very sparse.

        // TODO: give the user a descriptor to select the method to use?

        if (8 * anz < n)
        {

            //------------------------------------------------------------------
            // qsort method
            //------------------------------------------------------------------

            // memory usage is O(anz) and time is O(anz*log(anz)).  This is
            // more efficient than the bucket method, below, when A is very
            // hypersparse.  The time and memory complexity does not depend
            // on n.

            // since T is a GrB_Vector, it is not hypersparse
            GB_NEW (&T, ttype, n, 1, GB_Ap_null, true, GB_FORCE_NONHYPER,
                GB_HYPER_DEFAULT, 1, Context) ;
            GB_OK (info) ;

            // GB_build treats Ai and Ax as read-only; they must not be modified
            info = GB_build
            (
                T,                  // construct result in the T vector
                (GrB_Index *) Ai,   // indices inside the vector
                NULL,               // vector indices (none)
                Ax,                 // values, of size anz
                anz,                // number of tuples
                reduce,             // reduction operator
                acode,              // type code of the Ax array
                false,              // the input is a vector
                false,              // indices do not need to be checked
                Context
            ) ;

            if (info != GrB_SUCCESS)
            { 
                // out of memory
                GB_MATRIX_FREE (&T) ;
                return (info) ;
            }
            ASSERT (T->nvec_nonempty == GB_nvec_nonempty (T, NULL)) ;

        }
        else
        {

            //------------------------------------------------------------------
            // bucket method
            //------------------------------------------------------------------

            // Determine number of threads to use for constructing the buckets.
            // Each thread requires O(n) Sauna workspace, so this method does
            // not scale well when there are many threads compared to anz.
            // Total workspace is n*nth, so limit the # of threads used so that
            // at most anz workspace is used.

            int nth = anz / n ;
            nth = GB_IMIN (nth, nthreads) ;
            nth = GB_IMAX (nth, 1) ;

            //------------------------------------------------------------------
            // slice the entries for each thread
            //------------------------------------------------------------------

            // Thread tid does entries pstart_slice [tid] to
            // pstart_slice [tid+1]-1.  No need to compute kfirst or klast.

            int64_t pstart_slice [nth+1] ;
            GB_eslice (pstart_slice, anz, nth) ;

            //------------------------------------------------------------------
            // allocate Sauna workspace for each thread
            //------------------------------------------------------------------

            int Sauna_ids [nth] ;
            GB_Sauna Saunas [nth] ;
            GB_OK (GB_Sauna_acquire (nth, Sauna_ids, NULL, Context)) ;

            #undef  GB_FREE_ALL
            #define GB_FREE_ALL                                             \
            {                                                               \
                GB_MATRIX_FREE (&T) ;                                       \
                /* free and release all Sauna workspaces */                 \
                for (int tid = 0 ; tid < nth ; tid++)                       \
                {                                                           \
                    GB_Sauna_free (Sauna_ids [tid]) ;                       \
                }                                                           \
                GrB_Info info2 = GB_Sauna_release (nthreads, Sauna_ids) ;   \
                info = GB_IMAX (info, info2) ;                              \
            }

            bool ok = true ;

            #pragma omp parallel for num_threads(nth) schedule(static) \
                reduction(&:ok)
            for (int tid = 0 ; tid < nth ; tid++)
            {
                int Sauna_id = Sauna_ids [tid] ;
                GB_Sauna Sauna = GB_Global_Saunas_get (Sauna_id) ;
                if (Sauna == NULL || Sauna->Sauna_n < n
                    || Sauna->Sauna_size < zsize)
                { 
                    // get a new Sauna: the Sauna either does not exist,
                    // or is too small
                    GB_Sauna_free (Sauna_id) ;
                    GrB_Info my_info = GB_Sauna_alloc (Sauna_id, n, zsize) ;
                    ok = ok && (my_info == GrB_SUCCESS) ;
                    Sauna = GB_Global_Saunas_get (Sauna_id) ;
                }
                Saunas [tid] = Sauna ;
            }

            if (!ok)
            {
                // out of memory
                GB_FREE_ALL ;
                return (GB_OUT_OF_MEMORY) ;
            }

            //------------------------------------------------------------------
            // set hiwater of all Saunas to be the same
            //------------------------------------------------------------------

            int64_t hiwater = 0 ;
            for (int tid = 0 ; tid < nth ; tid++)
            {
                GB_Sauna Sauna = Saunas [tid] ;
                int64_t my_hiwater = GB_Sauna_reset (Sauna, 1, 0) ;
                hiwater = GB_IMAX (hiwater, my_hiwater) ;
            }

            for (int tid = 0 ; tid < nth ; tid++)
            {
                GB_Sauna Sauna = Saunas [tid] ;
                Sauna->Sauna_hiwater = hiwater ;
            }

            //------------------------------------------------------------------
            // sum across each index: T(i) = reduce (A (i,:))
            //------------------------------------------------------------------

            // Early exit cannot be exploited; ignore the terminal value.

            #undef  GB_red
            #define GB_red(opname,aname) GB_red_eachindex_ ## opname ## aname
            #undef  GB_RED_WORKER
            #define GB_RED_WORKER(opname,aname,atype)                       \
            {                                                               \
                info = GB_red (opname, aname) (&T, ttype, Saunas, hiwater,  \
                    A, pstart_slice, nth, nthreads, Context) ;              \
                done = true ;                                               \
            }                                                               \
            break ;

            bool done = false ;

            //------------------------------------------------------------------
            // launch the switch factory
            //------------------------------------------------------------------

            #ifndef GBCOMPACT

                if (nocasting)
                { 
                    // controlled by opcode and typecode.  No typecasting
                    GB_Opcode opcode = reduce->opcode ;
                    GB_Type_code typecode = acode ;
                    ASSERT (typecode <= GB_UDT_code) ;
                    #include "GB_red_factory.c"
                    GB_OK (info) ;
                }

            #endif

            //------------------------------------------------------------------
            // generic worker
            //------------------------------------------------------------------

            if (!done)
            {
                #include "GB_reduce_each_index.c"
            }

            //------------------------------------------------------------------
            // release all Saunas
            //------------------------------------------------------------------

            GB_OK (GB_Sauna_release (nth, Sauna_ids)) ;
        }
        ASSERT_OK (GB_check (T, "T output for T = reduce_each_index (A)", GB0));
    }

    //--------------------------------------------------------------------------
    // C<M> = accum (C,T): accumulate the results into C via the mask
    //--------------------------------------------------------------------------

    return (GB_accum_mask (C, M, NULL, accum, &T, C_replace, Mask_comp,
        Context)) ;
}

