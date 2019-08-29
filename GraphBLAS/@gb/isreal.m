function s = isreal (G)
%ISREAL true for real GraphBLAS matrices.
%
% See also isnumeric, isfloat, isinteger, islogical, gb.type, isa, gb.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

s = ~isequal (gbtype (G.opaque), 'complex') ;

