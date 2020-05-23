function C = asinh (G)
%ASINH inverse hyperbolic sine.
% C = asinh (G) computes the inverse hyberbolic sine of each entry of a
% GraphBLAS matrix G.
%
% See also GrB/sin, GrB/asin, GrB/sinh.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2020, All Rights
% Reserved. http://suitesparse.com.  See GraphBLAS/Doc/License.txt.

if (~isfloat (G))
    G = GrB (G, 'double') ;
end

C = GrB.apply ('asinh', G) ;
