//#include <vector>

// Jon's maths library :D
namespace jml
{
    template <typename T>
    Matrix<T> Real(const Matrix<std::complex<T>> &A)
    {
        Matrix<T> tmp(A.rows(), A.cols());

//#pragma omp parallel for
        for(int i = 0; i < A.size(); ++i)
            tmp(i) = A(i).real();

        return tmp;
    }
}

template <typename T>
std::vector<T>& Multiply(const Matrix<T> &A, const std::vector<T> &b, std::vector<T> &c)
{
    int M = A.rows();
    int N = A.cols();

    assert(b.size() == N);

    c.resize(M);
    T sum;
    const T *pRow, *pCol;

    for (int i = 0; i<M; i++)
    {
        pRow = &A(i, 0);
        pCol = &b[0];
        sum = 0;

        for (int j = 0; j<N; ++j)
        {
            sum += (*pRow) * (*pCol);
            pRow++;
            pCol++;
        }
        c[i] = sum;
    }
    return c;
}

template<typename T>
std::vector<T> operator*(const Matrix<T> &A, const std::vector<T> &b)
{
    assert(A.cols() == b.size());

    int rows = A.rows();
    std::vector<T> tmp(rows);
    Multiply(A, b, tmp);

    return tmp;
}

template<typename T, typename X>
Matrix<T> operator*(const Matrix<T> &A, const X b)
{
    // can we assert X must de number type
    Matrix<T> tmp(A.rows(), A.cols());

    for(int i = 0; i < A.size(); ++i)
        tmp(i) = A(i) * b;

    return tmp;
}

template<typename T, typename X>
Matrix<T> operator*(const X b, const Matrix<T> &A)
{
    // can we assert X must de number type
    Matrix<T> tmp(A.rows(), A.cols());

    for(int i = 0; i < A.size(); ++i)
        tmp(i) = A(i) * b;

    return tmp;
}

template<typename T>
Matrix<T> operator+(const Matrix<T> &A, const Matrix<T> &B)
{
    assert(A.cols() == B.cols() && A.rows() == B.rows());

    Matrix<T> tmp(A.rows(), A.cols());

    for(int i = 0; i < A.size(); ++i)
        tmp(i) = A(i) + B(i);

    return tmp;
}

template <typename T>
Matrix<T> Transpose(const Matrix<T> &A)
{
    int rows = A.cols();
    int cols = A.rows();

    Matrix<T> tmp(rows, cols);
    for (int i = 0; i<rows; ++i)
        for (int j = 0; j<cols; ++j)
            tmp(i, j) = A(j, i);

    return tmp;
}

// Preform transpose multiplication (i.e. A^T * B)
template <typename T>
Matrix<T> TransposeMultiply(const Matrix<T> &A, const Matrix<T> &B)
{
    assert(A.rows() == B.rows());

    int rows = A.cols();
    int columns = B.cols();
    int K = A.rows();

    Matrix<T> tmp(rows, columns);
    for (int i = 0; i<rows; ++i)
        for (int j = 0; j<columns; ++j)
            for (int k = 0; k<K; ++k)
                tmp(i, j) += A(k, i) * B(k, j);

    return tmp;
}

// Perform transpose multiplication (i.e. A^T * B)
template <typename T>
std::vector<T> TransposeMultiply(const Matrix<T> &A, const std::vector<T> &v)
{
    assert(A.rows() == v.size());

    int rows = A.rows();
    int columns = A.cols();

    std::vector<T> tmp(columns);
    for (int i = 0; i<columns; ++i)
        for (int j = 0; j<rows; ++j)
            tmp[i] += A(j, i) * v[j];

    return tmp;
}

// Perform complex transpose multiplication (i.e. A^T * B)
template <typename T>
Matrix<std::complex<T> > TransposeMultiply(const Matrix<std::complex<T> > &A, const Matrix<std::complex<T> > &B)
{
    assert(A.rows() == B.rows());

    int rows = A.cols();
    int columns = B.cols();
    int K = A.rows();

    Matrix<std::complex<T> > tmp(rows, columns);
    for (int i = 0; i<rows; ++i)
        for (int j = 0; j<columns; ++j)
            for (int k = 0; k<K; ++k)
                tmp(i, j) += std::conj(A(k, i)) * B(k, j);

    return tmp;
}

// Perform copmplex transpose multiplication (i.e. A^T * B)
template <typename T>
std::vector<std::complex<T> > TransposeMultiply(const Matrix<std::complex<T> > &A, const std::vector<std::complex<T> > &v)
{
    assert(A.rows() == v.size());

    int rows = A.rows();
    int columns = A.cols();

    std::vector<std::complex<T> > tmp(columns);
    for (int i = 0; i<columns; ++i)
        for (int j = 0; j<rows; ++j)
            tmp[i] += std::conj(A(j, i)) * v[j];

    return tmp;
}

template <typename T>
void swap(Matrix<T> &LHS, Matrix<T> &RHS)
{
    int m = LHS.rows();
    int n = LHS.cols();

    assert(m == RHS.rows());
    assert(n == RHS.cols());

//#pragma omp parallel for collapse(2)
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < n; ++j)
            std::swap(LHS(i, j), RHS(i, j));
}

// Preform transpose multiplication (i.e. A^T * B)
template <typename T>
Matrix<T> Inner(const Matrix<T> &A, const Matrix<T> &B)
{
    assert(A.cols() == B.rows());

    Matrix<T> tmp(A.rows(), B.cols(), 0.0);
    for (int i = 0; i < A.rows(); ++i)
        for (int j = 0; j < B.cols(); ++j)
            for (int k = 0; k < A.cols(); ++k)
                tmp(i, j) += A(i, k) * B(k, j);

    return tmp;
}
