/*
   Copyright (c) 2009-2014, Jack Poulson
   All rights reserved.

   This file is part of Elemental and is under the BSD 2-Clause License, 
   which can be found in the LICENSE file in the root directory, or at 
   http://opensource.org/licenses/BSD-2-Clause
*/
#pragma once
#ifndef EL_RQ_SOLVEAFTER_HPP
#define EL_RQ_SOLVEAFTER_HPP

// TODO: Extend for BusingerGolub support

namespace El {
namespace rq {

template<typename F> 
void SolveAfter
( Orientation orientation, 
  const Matrix<F>& A, const Matrix<F>& t, 
  const Matrix<Base<F>>& d, const Matrix<F>& B,       
        Matrix<F>& X )
{
    DEBUG_ONLY(CallStackEntry cse("rq::SolveAfter"))
    const Int m = A.Height();
    const Int n = A.Width();
    if( m > n )
        LogicError("Must have full row rank");
    // TODO: Add scaling
    auto AR = LockedView( A, IndexRange(0,m), IndexRange(n-m,n) );
    if( orientation == NORMAL )
    {
        if( m != B.Height() )
            LogicError("A and B do not conform");

        // Copy B into X
        X.Resize( n, B.Width() );
        Matrix<F> XT, XB;
        PartitionDown( X, XT, XB, m );
        XT = B;
        Zero( XB );

        // Solve against R (checking for singularities)
        Trsm( LEFT, UPPER, NORMAL, NON_UNIT, F(1), AR, XT, true );

        // Apply Q' to X 
        rq::ApplyQ( LEFT, ADJOINT, A, t, d, X );
    }
    else // orientation in {TRANSPOSE,ADJOINT}
    {
        if( n != B.Height() )
            LogicError("A and B do not conform");

        // Copy B into X
        X = B;

        if( orientation == TRANSPOSE )
            Conjugate( X );

        // Apply Q to X
        rq::ApplyQ( LEFT, NORMAL, A, t, d, X );

        // Shrink X to its new height
        X.Resize( m, X.Width() );

        // Solve against R' (check for singularities)
        Trsm( LEFT, UPPER, ADJOINT, NON_UNIT, F(1), AR, X, true );

        if( orientation == TRANSPOSE )
            Conjugate( X );
    }
}

template<typename F>
void SolveAfter
( Orientation orientation,
  const AbstractDistMatrix<F      >& APre, const AbstractDistMatrix<F>& t, 
  const AbstractDistMatrix<Base<F>>& d,    const AbstractDistMatrix<F>& B, 
        AbstractDistMatrix<F      >& XPre )
{
    DEBUG_ONLY(CallStackEntry cse("lq::SolveAfter"))
    const Int m = APre.Height();
    const Int n = APre.Width();
    const Grid& g = APre.Grid();
    if( m > n )
        LogicError("Must have full row rank");

    // Proxies cannot be resized since they might be views
    XPre.Resize( n, B.Width() );

    DistMatrix<F> A(g), X(g);
    Copy( APre, A, READ_PROXY );
    Copy( XPre, X, WRITE_PROXY );

    // TODO: Add scaling

    auto AR = LockedView( A, IndexRange(0,m), IndexRange(n-m,n) );
    if( orientation == NORMAL )
    {
        if( m != B.Height() )
            LogicError("A and B do not conform");

        // Copy B into X
        DistMatrix<F> XT(g), XB(g);
        PartitionDown( X, XT, XB, m );
        XT = B;
        Zero( XB );

        if( orientation == TRANSPOSE )
            Conjugate( XT );

        // Solve against R (checking for singularities)
        Trsm( LEFT, UPPER, NORMAL, NON_UNIT, F(1), AR, XT, true );

        // Apply Q' to X 
        rq::ApplyQ( LEFT, ADJOINT, A, t, d, X );

        if( orientation == TRANSPOSE )
            Conjugate( X );
    }
    else
    {
        // Copy B into X
        X = B;

        if( orientation == TRANSPOSE )
            Conjugate( X );

        // Apply Q to X
        rq::ApplyQ( LEFT, NORMAL, A, t, d, X );

        // Shrink X to its new height
        X.Resize( m, X.Width() );

        // Solve against R' (check for singularities)
        Trsm( LEFT, UPPER, ADJOINT, NON_UNIT, F(1), AR, X, true );

        if( orientation == TRANSPOSE )
            Conjugate( X );
    }
    Copy( X, XPre, RESTORE_WRITE_PROXY );
}

} // namespace rq
} // namespace El

#endif // ifndef EL_RQ_SOLVEAFTER_HPP
