//------------------------------------------------------------------------------
// GB_cuda_stringify_semiring: build strings for a semiring
//------------------------------------------------------------------------------

// SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2021, All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

//------------------------------------------------------------------------------

// Construct a string defining all macros for semiring, and its name.
// User-defined types are not handled.

#include "GB.h"
#include "GB_cuda_stringify.h"

// TODO: need the actual C, M, A and B matrices, or at least something to
// determine their sparsity structures for GB_cuda_enumify_sparsity.

//------------------------------------------------------------------------------
// GB_cuda_stringify_semiring: build strings for a semiring
//------------------------------------------------------------------------------

void GB_cuda_stringify_semiring     // build a semiring (name and code)
(
    // input:
    GrB_Semiring semiring,  // the semiring to stringify
    bool flipxy,            // multiplier is: mult(a,b) or mult(b,a)
    GrB_Type ctype,         // the type of C
    GrB_Type atype,         // the type of A
    GrB_Type btype,         // the type of B
    GrB_Type mtype,         // the type of M, or NULL if no mask
    bool Mask_struct,       // mask is structural
    bool Mask_comp,         // mask is complemented

    // output: (all of size at least GB_CUDA_STRLEN+1)
    char *semiring_name,    // name of the semiring
    char *semiring_code,    // List of types and macro defs
    char *mask_name         // definition of mask data load
)
{
    uint64_t scode ;

    GB_cuda_enumify_semiring (&scode,
        semiring, flipxy, ctype, atype, btype, mtype, Mask_struct, Mask_comp) ;
    GB_cuda_macrofy_semiring (semiring_name, semiring_code, mask_name, scode) ;
}

//------------------------------------------------------------------------------
// GB_cuda_enumify_semiring: enumerate a semiring
//------------------------------------------------------------------------------

void GB_cuda_enumify_semiring   // enumerate a semiring
(
    // output:
    uint64_t *scode,        // unique encoding of the entire semiring
    // input:
    GrB_Semiring semiring,  // the semiring to enumify
    bool flipxy,            // multiplier is: mult(a,b) or mult(b,a)
    GrB_Type ctype,         // the type of C
    GrB_Type atype,         // the type of A
    GrB_Type btype,         // the type of B
    GrB_Type mtype,         // the type of M, or NULL if no mask
    bool Mask_struct,       // mask is structural
    bool Mask_comp          // mask is complemented
)
{

    //--------------------------------------------------------------------------
    // check inputs
    //--------------------------------------------------------------------------

    ASSERT (semiring->object_kind == GB_BUILTIN) ;

    //--------------------------------------------------------------------------
    // get the semiring
    //--------------------------------------------------------------------------

    GrB_Monoid add = semiring->add ;
    GrB_BinaryOp mult = semiring->multiply ;
    GrB_BinaryOp addop = add->op ;
    GrB_Type xtype = mult->xtype ;
    GrB_Type ytype = mult->ytype ;
    GrB_Type ztype = mult->ztype ;
    GB_Opcode mult_opcode = mult->opcode ;
    GB_Opcode add_opcode  = addop->opcode ;
    GB_Type_code xcode = xtype->code ;
    GB_Type_code ycode = ytype->code ;
    GB_Type_code zcode = ztype->code ;

    // these must always be true for any semiring:
    ASSERT (mult->ztype == addop->ztype) ;
    ASSERT (addop->xtype == addop->ztype && addop->ytype == addop->ztype) ;

    //--------------------------------------------------------------------------
    // rename redundant boolean operators
    //--------------------------------------------------------------------------

    // consider z = op(x,y) where both x and y are boolean:
    // DIV becomes FIRST
    // RDIV becomes SECOND
    // MIN and TIMES become LAND
    // MAX and PLUS become LOR
    // NE, ISNE, RMINUS, and MINUS become LXOR
    // ISEQ becomes EQ
    // ISGT becomes GT
    // ISLT becomes LT
    // ISGE becomes GE
    // ISLE becomes LE

    if (zcode == GB_BOOL_code)
    {
        // rename the monoid
        add_opcode = GB_boolean_rename (add_opcode) ;
    }

    if (xcode == GB_BOOL_code)  // && (ycode == GB_BOOL_code)
    { 
        // rename the multiplicative operator
        mult_opcode = GB_boolean_rename (mult_opcode) ;
    }

    //--------------------------------------------------------------------------
    // handle the flip
    //--------------------------------------------------------------------------

    if (flipxy)
    { 
        // z = fmult (b,a) will be computed: handle this by renaming the
        // multiplicative operator, if possible.

        // handle the flip
        bool handled ;
        mult_opcode = GB_binop_flip (mult_opcode, &handled) ;

        if (handled)
        {
            // the flip is now handled completely.
            flipxy = false ;
        }
    }

    // If flipxy is still true, then the multiplier must be used as fmult(b,a)
    // inside the semiring, since it has no flipped equivalent.

    //--------------------------------------------------------------------------
    // determine if A and/or B are value-agnostic
    //--------------------------------------------------------------------------

    // These 1st, 2nd, and pair operators are all handled by the flip, so if
    // flipxy is still true, all of these booleans will be false.
    bool op_is_first  = (mult_opcode == GB_FIRST_opcode ) ;
    bool op_is_second = (mult_opcode == GB_SECOND_opcode) ;
    bool op_is_pair   = (mult_opcode == GB_PAIR_opcode) ;
    bool A_is_pattern = op_is_second || op_is_pair ;
    bool B_is_pattern = op_is_first  || op_is_pair ;

    //--------------------------------------------------------------------------
    // enumify the multiplier
    //--------------------------------------------------------------------------

    int mult_ecode ;
    GB_cuda_enumify_binop (&mult_ecode, mult_opcode, xcode, true) ;

    //--------------------------------------------------------------------------
    // enumify the monoid
    //--------------------------------------------------------------------------

    int add_ecode ;
    GB_cuda_enumify_binop (&add_ecode, add_opcode, zcode, false) ;
    ASSERT (add_ecode < 32) ;

    int id_ecode ;
    GB_cuda_enumify_identity (&id_ecode, add_opcode, zcode) ;

    int term_ecode ;
    bool is_term ;
    GB_cuda_enumify_terminal (&is_term, &term_ecode, add_opcode, zcode) ;

    //--------------------------------------------------------------------------
    // enumify the types
    //--------------------------------------------------------------------------

    int acode = A_is_pattern ? 0 : atype->code ;   // 0 to 14
    int bcode = B_is_pattern ? 0 : btype->code ;   // 0 to 14
    int ccode = ctype->code ;                      // 1 to 14

    //--------------------------------------------------------------------------
    // enumify the mask
    //--------------------------------------------------------------------------

    int mtype_code = (mtype == NULL) ? 0 : mtype->code ; // 0 to 14
    int mask_ecode ;
    GB_cuda_enumify_mask (&mask_ecode, mtype_code, Mask_struct, Mask_comp) ;

    //--------------------------------------------------------------------------
    // construct the semiring scode
    //--------------------------------------------------------------------------

    #define LSHIFT(x,k) (((uint64_t) x) << k)

    (*scode) =
                                             // range        bits
                // monoid
                LSHIFT (add_ecode  , 55) |  // 0 to 22      5
                LSHIFT (id_ecode   , 50) |  // 0 to 31      5
                LSHIFT (term_ecode , 45) |  // 0 to 31      5

                // multiplier, z = f(x,y) or f(y,x)
                LSHIFT (mult_ecode , 37) |  // 0 to 139     8
                LSHIFT (flipxy     , 36) |  // 0 to 1       1
                LSHIFT (zcode      , 32) |  // 0 to 14      4
                LSHIFT (xcode      , 28) |  // 0 to 14      4
                LSHIFT (ycode      , 24) |  // 0 to 14      4

                // mask
                LSHIFT (mask_ecode , 20) |  // 0 to 13      4

                // types of C, A, and B (bool, int*, uint*, etc)
                LSHIFT (ccode      , 16) |  // 1 to 14      4
                LSHIFT (acode      , 12) |  // 0 to 14      4
                LSHIFT (bcode      ,  8) |  // 0 to 14      4

                // formats of C, A, and B (sparse, hyper, bitmap, or full)
                LSHIFT (csparsity  ,  6) |  // 0 to 3       2
                LSHIFT (msparsity  ,  4) |  // 0 to 3       2
                LSHIFT (asparsity  ,  2) |  // 0 to 3       2
                LSHIFT (bsparsity  ,  0) ;  // 0 to 3       2

    // total scode bits: 60
}


void GB_cuda_macrofy_semiring   // construct all macros for a semiring
(
    // output:
    char *semiring_name,    // name of the semiring
    char *semiring_code,    // List of types and macro defs
    char *mask_name,        // definition of mask data load
    // input:
    uint64_t scode
)
{

    //--------------------------------------------------------------------------
    // extract the semiring scode
    //--------------------------------------------------------------------------

    #define RSHIFT(x,k,b) (x >> k) && ((((uint64_t) 1) << b) - 1)

    // monoid
    int add_ecode   = RSHIFT (scode, 55, 5) ;
    int id_ecode    = RSHIFT (scode, 50, 5) ;
    int term_ecode  = RSHIFT (scode, 45, 5) ;
    bool is_term    = (term_ecode == 31) ;

    // multiplier
    int mult_ecode  = RSHIFT (scode, 37, 8) ;
    bool flipxy     = RSHIFT (scode, 36, 1) ;
    int zcode       = RSHIFT (scode, 32, 4) ;
    int xcode       = RSHIFT (scode, 28, 4) ;
    int ycode       = RSHIFT (scode, 24, 4) ;

    // mask
    int mask_ecode  = RSHIFT (scode, 20, 4) ;

    // types of C, A, and B
    int acode       = RSHIFT (scode, 16, 4) ;
    int bcode       = RSHIFT (scode, 12, 4) ;
    int ccode       = RSHIFT (scode,  8, 4) ;

    // formats of C, A, and B
    int csparsity   = RSHIFT (scode,  6, 2) ;
    int msparsity   = RSHIFT (scode,  4, 2) ;
    int asparsity   = RSHIFT (scode,  2, 2) ;
    int bsparsity   = RSHIFT (scode,  0, 2) ;

    //--------------------------------------------------------------------------
    // construct macros to load scalars from A and B (and typecast) them
    //--------------------------------------------------------------------------

    // TODO: these need to be typecasted when loaded.
    // if flipxy false:  A is typecasted to x, and B is typecasted to y.
    // if flipxy true:   A is typecasted to y, and B is typecasted to x.

    bool A_is_pattern = (acode == 0) ;
    bool B_is_pattern = (bcode == 0) ;

    char acast_macro [GB_CUDA_STRLEN+1] ;
    char bcast_macro [GB_CUDA_STRLEN+1] ;
    GB_cuda_stringify_load (acast_macro, "GB_GETA", A_is_pattern) ;
    GB_cuda_stringify_load (bcast_macro, "GB_GETB", B_is_pattern) ;

    //--------------------------------------------------------------------------
    // construct macros for the multiply
    //--------------------------------------------------------------------------

    char s [GB_CUDA_STRLEN+1] ;

    char mult_macro [GB_CUDA_STRLEN+1] ;
//  GB_cuda_stringify_binop (mult_macro, "GB_MULT", mult_opcode, zcode, true) ;
    GB_cuda_charify_binop (&s, mult_ecode) ;
    GB_cuda_macrofy_binop (mult_macro, "GB_MULT", s, flipxy) ;

    //--------------------------------------------------------------------------
    // construct the monoid macros
    //--------------------------------------------------------------------------

    char add_macro [GB_CUDA_STRLEN+1] ;
//  GB_cuda_stringify_binop (add_macro, "GB_ADD", add_opcode, zcode, false) ;
    GB_cuda_charify_binop (&s, add_ecode) ;
    GB_cuda_macrofy_binop (add_macro, "GB_ADD", s) ;

    char identity_macro [GB_CUDA_STRLEN+1] ;
//  GB_cuda_stringify_identity (identity_macro, add_opcode, zcode) ;
    GB_cuda_charify_identity_or_terminal (&s, id_ecode) ;
    GB_cuda_macrofy_identity (identity_macro, s) ;

    char terminal_expression_macro [GB_CUDA_STRLEN+1] ;
    char terminal_statement_macro  [GB_CUDA_STRLEN+1] ;

//  GB_cuda_stringify_terminal (&is_term,
//      terminal_expression_macro, terminal_statement_macro,
//      "GB_TERMINAL_CONDITION", "GB_IF_TERMINAL_BREAK", add_opcode, zcode) ;

    char texpr [GB_CUDA_STRLEN+1] ;
    char tstmt [GB_CUDA_STRLEN+1] ;

    // convert ecode and is_term to strings
    GB_cuda_charify_identity_or_terminal (&s, ecode) ;
    GB_cuda_charify_terminal_expression (texpr, s, is_term, ecode) ;
    GB_cuda_charify_terminal_statement  (tstmt, s, is_term, ecode) ;

    // convert strings to macros
    GB_cuda_macrofy_terminal_expression (terminal_expression_macro,
        "GB_TERMINAL_CONDITION", texpr) ;
    GB_cuda_macrofy_terminal_statement (terminal_statement_macro,
        "GB_IF_TERMINAL_BREAK", tstmt) ;

    //--------------------------------------------------------------------------
    // macro to typecast the result back into C
    //--------------------------------------------------------------------------

    // for the ANY_PAIR semiring, "c_is_one" will be true, and Cx [0..cnz] will
    // be filled with all 1's later.
    bool c_is_one = false ;
    // TODO:
    // (add_ecode == GB_ANY_opcode && mult_opcode == GB_PAIR_opcode) ;
    char ccast_macro [GB_CUDA_STRLEN+1] ;
    GB_cuda_stringify_load (ccast_macro, "GB_PUTC", c_is_one) ;

    //--------------------------------------------------------------------------
    // construct the macros to access the mask (if any), and its name
    //--------------------------------------------------------------------------

    const char *mask_macros = "" ;
    GB_cuda_macrofy_mask (&mask_macros, mask_ecode) ;
    snprintf (mask_name, GB_CUDA_STRLEN, "mask_%d", mask_ecode) ;

    //--------------------------------------------------------------------------
    // determine the sparsity formats of C, M, A, and B
    //--------------------------------------------------------------------------

    //--------------------------------------------------------------------------
    // build the final semiring code
    //--------------------------------------------------------------------------

    snprintf (semiring_code, GB_CUDA_STRLEN,
        "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
        acast_macro, bcast_macro, mult_macro, add_macro, identity_macro,
        terminal_expression_macro, terminal_statement_macro,
        ccast_macro, mask_macros,
        // TODO: C, M, A, and B sparsity formats
        ) ;

    //--------------------------------------------------------------------------
    // build the final semiring name
    //--------------------------------------------------------------------------

    snprintf (semiring_name, "semiring_%0.16X", scode) ;

    printf ("semiring_name:\n%s\n", semiring_name) ;
    //printf ("semiring_code:\n%s\n", semiring_code) ;
    //printf ("mask_name:    \n%s\n", mask_name) ;
}

