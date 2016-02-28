#ifndef REGRESSION_H
#define REGRESSION_H

#include<vector>

#include "matrix.h"

namespace Regression
{

    //ripped from http://www.codeproject.com/Articles/25335/An-Algorithm-for-Weighted-Linear-Regression
    static bool SymmetricMatrixInvert(Matrix<double>& V)
    {
        //if (V.cols() != V.rows())
            //throw

        int N = (int)std::sqrt(V.size());
        std::vector<double> t(N);
        std::vector<double> Q(N);
        std::vector<double> R(N);
        double AB;
        int K, L, M;

        for (M = 0; M < N; ++M)
            R[M] = 1;
        K=0;
        for (M = 0; M < N; ++M)
        {
            double Big = 0;
            for (L = 0 ; L < N; ++L)
            {
                AB = std::fabs(V(L, L));
                if (AB > Big && R[L] != 0)
                {
                    Big = AB;
                    K = L;
                }
            }
            if (Big == 0)
            {
                return false;
            }
            R[K] = 0;
            Q[K] = 1 / V(K, K);
            t[K] = 1;
            V(K, K) = 0;
            if (K != 0)
            {
                for (L = 0; L < K; ++L)
                {
                    t[L] = V(L, K);
                    if (R[L] == 0)
                        Q[L] = V(L, K) * Q[K];
                    else
                        Q[L] = -V(L, K) * Q[K];
                    V(L, K) = 0;
                }
            }
            if (K + 1 < N)
            {
                for (L = K + 1; L < N; ++L)
                {
                    if(R[L] != 0)
                        t[L] = V(K, L);
                    else
                        t[L] = -V(K, L);
                    Q[L] = -V(K, L) * Q[K];
                    V(K, L) = 0;
                }
            }
            for (L = 0; L < N; ++L)
                for (K = L; K < N; ++K)
                    V(L, K) = V(L, K) + t[L] * Q[K];
        }
        M = N;
        L = N - 1;
        for (K = 1; K < N; ++K)
        {
            M = M - 1;
            L = L - 1;
            for (int J = 0; J <= L; ++J)
                V(M, J) = V(J, M);
        }
        return true;
    }

    //ripped from http://www.codeproject.com/Articles/25335/An-Algorithm-for-Weighted-Linear-Regression
    static bool LinearRegression(std::vector<double> Y, Matrix<double> X, std::vector<double> W, std::vector<double> &C)
    {
        int M = Y.size();
        int N = X.size() / M;
        int NDF = M - N;
        std::vector<double> Ycalc(M);
        std::vector<double> DY(M);

        if (NDF < 1)
            return false;

        Matrix<double> V(N, N);
        C = std::vector<double>(N);
        std::vector<double> B(N);

        //everything should be 0

        for (int i = 0; i < N; ++i)
        {
            for (int j = 0; j < N; ++j)
                for (int k = 0; k < M; ++k)
                    V(i, j) = V(i, j) + W[k] * X(i, k) * X(j, k);
            for (int k = 0; k < M; ++k)
                B[i] = B[i] + W[k] * X(i, k) * Y[k];
        }

        if (!SymmetricMatrixInvert(V))
            return false;

        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                C[i] = C[i] + V(i, j) * B[j];

        // don't need the rest
    }

}

#endif // REGRESSION_H

