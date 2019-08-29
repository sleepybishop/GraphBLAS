function C = power (A, B)
%.^ Array power.
% C = A.^B computes element-wise powers.  One or both of A and B may be
% scalars.  Otherwise, A and B must have the same size.  The computation
% takes O(m*n) time if the matrices are m-by-n, except in the case that
% B is a positive scalar (greater than zero).  In that case, the pattern
% of C is a subset of the pattern of A.
%
% Note that complex matrices are not yet supported.

% SuiteSparse:GraphBLAS, Timothy A. Davis, (c) 2017-2019, All Rights Reserved.
% http://suitesparse.com   See GraphBLAS/Doc/License.txt for license.

if (isscalar (A))
    if (isscalar (B))
        % both A and B are scalars
        A = full (A) ;
        B = full (B) ;
    else
        % A is a scalar, B is a matrix; expand A to the size of B
        A = gb.expand (A, true (size (B))) ;
        B = full (B) ;
    end
else
    if (isscalar (B))
        % A is a matrix, B is a scalar
        if (get_scalar (B) <= 0)
            % so the result is full
            A = full (A) ;
            B = gb.expand (B, true (size (A))) ;
        else
            % The scalar b is > 0, and thus 0.^b is zero.  The result is
            % sparse.  B is expanded to a matrix wit the same pattern as A.
            B = gb.expand (B, A) ;
        end
    else
        % both A and B are matrices.
        A = full (A) ;
        B = full (B) ;
    end
end

% GraphBLAS does not have a binary operator f(x,y)=x^y.  It could be
% constructed as a user-defined operator, but this is reasonably fast.
% FUTURE: create a binary operator f(x,y) = x^y.
[m, n] = size (A) ;
[I, J, Ax] = gb.extracttuples (A) ;
[I, J, Bx] = gb.extracttuples (B) ;
C = gb.select ('nonzero', gb.build (I, J, (Ax .^ Bx), m, n)) ;
end

