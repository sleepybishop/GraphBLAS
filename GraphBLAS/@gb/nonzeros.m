function X = nonzeros (G)
%NONZEROS extract entries from a GraphBLAS matrix.
% X = nonzeros (G) extracts the entries from a GraphBLAS matrix G.  X has
% the same type as G ('double', 'single', 'int8', ...).  If G contains
% explicit entries with a value of zero, these are dropped from X.  Use
% gb.extracttuples to return those entries.
%
% See also gb.extracttuples, find.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

X = gbextractvalues (gbselect ('nonzero', G.opaque, struct ('kind', 'gb'))) ;

