function s = issparse (G)
%ISSPARSE always true for any GraphBLAS matrix.
% issparse (G) is always true for any GraphBLAS matrix G.
%
% See also ismatrix, isvector, isscalar, sparse, full, isa, gb.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

s = true ;

