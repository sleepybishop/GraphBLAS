function C = round (G)
%ROUND round entries to the nearest integers
% See also ceil, floor, fix.

% FUTURE: this could be much faster as a mexFunction.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

if (isfloat (G) && gb.nvals (G) > 0)
    [m n] = size (G) ;
    [i j x] = gb.extracttuples (G, struct ('kind', 'zero-based')) ;
    C = gb.build (i, j, round (x), m, n) ;
else
    C = G ;
end

