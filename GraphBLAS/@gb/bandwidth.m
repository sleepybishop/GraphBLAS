function [arg1,arg2] = bandwidth (G,uplo)
%BANDWIDTH Determine the bandwidth of a GraphBLAS matrix.
% [lo, hi] = bandwidth (G) returns the upper and lower bandwidth of G.
% lo = bandwidth (G, 'lower') returns just the lower bandwidth.
% hi = bandwidth (G, 'upper') returns just the upper bandwidth.
%
% See also isbanded, isdiag, istril, istriu.

% FUTURE: this will be much faster when implemented in a mexFunction.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

if (gb.nvals (G) == 0)
    % matrix is empty
    hi = 0 ;
    lo = 0 ;
else
    [i j] = gb.extracttuples (G, struct ('kind', 'zero-based')) ;
    b = j - i ;
    hi = max (0,  double (max (b))) ;
    lo = max (0, -double (min (b))) ;
end
if (nargin == 1)
   arg1 = lo ;
   arg2 = hi ;
else
    if (nargout > 1)
        error ('too many output arguments') ;
    elseif isequal (uplo, 'lower')
        arg1 = lo ;
    elseif isequal (uplo, 'upper')
        arg1 = hi ;
    else
        error ('unrecognized input parameter') ;
    end
end

