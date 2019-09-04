function gbtest33
%GBTEST33 test spones, numel, nzmax, size, length, isempty, issparse, ...
% ismatrix, isvector, isscalar, isnumeric, isfloat, isreal, isinteger,
% islogical, isa

rng ('default') ;

fprintf ('gbtest33:\n') ;

types = gbtest_types ;

for k1 = 1:length(types)
    type = types {k1} ;
    fprintf ('%s ', type) ;

    for k2 = 1:length(types)
        type2 = types {k2} ;

        for n = 0:3
            for m = 0:3
                A = 100 * rand (m, n) ;
                A (A < 50) = 0 ;
                S = sparse (A) ;

                G = gb (S, type) ;
                G2 = spones (G, type2) ;
                assert (isequal (gb.type (G2), type2)) ;

                C = double (G2) ;
                assert (isequal (sparse (C), spones (S))) ;

                assert (numel (G) == m*n) ;
                assert (nzmax (G) == max (nnz (G), 1))
                assert (isequal (size (G), [m n])) ;
                [m1 n1]  = size (G) ;
                assert (isequal ([m1 n1], [m n])) ;
                if (m == 0 | n == 0)
                    assert (length (G) == 0) ;
                else
                    assert (length (G) == max (m, n)) ;
                end
                assert (isempty (G) == (m == 0 | n == 0)) ;
                assert (issparse (G)) ;
                assert (issparse (full (G))) ;
                assert (ismatrix (G)) ;
                assert (isnumeric (G)) ;
                assert (isvector (G) == (m == 1 | n == 1)) ;
                assert (isscalar (G) == (m == 1 & n == 1)) ;

                isfl = isequal (type, 'double') | isequal (type, 'single') | ...
                       isequal (type, 'complex') ;
                assert (isfloat (G) == isfl) ;
                assert (isreal (G) == (~isequal (type, 'complex'))) ;
                isint = isequal (type (1:3), 'int') | ...
                        isequal (type (2:4), 'int') ;
                assert (isinteger (G) == isint) ;
                islog = isequal (type, 'logical') ;
                assert (islogical (G) == islog) ;

                assert (isa (G, 'gb')) ;
                assert (isa (G, 'numeric')) ;
                assert (isa (G, 'float') == isfl) ;
                assert (isa (G, 'integer') == isint) ;
                assert (isa (G, 'logical') == islog) ;
                assert (isa (G, type) == isequal (gb.type (G), type)) ;
            end
        end
    end
end

fprintf ('\ngbtest33: all tests passed\n') ;