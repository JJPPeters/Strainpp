#ifndef MATRIX_H
#define MATRIX_H


#include<complex>
#include<vector>
#include <cassert>

// Very simple implementation of a 2D matrix using std::vector as the main container.
template <typename T>
class Matrix
{
public:
    // Default constructor, everything 0
    Matrix();

    // Creates Matrix as copy of another matrix
    Matrix(const Matrix<T> &RHS);

    // Creates Matrix of size (rows*cols) filled with zeros
    Matrix(int rows, int cols);

    // Creates Matrix of size (rows*cols) filled with scalar
    Matrix(int rows, int cols, const T &scalar);

    // Creates Matrix of size (rows*cols) filled with values in data vector
    Matrix(int rows, int cols, const std::vector<T> &InData);

    // Default Destructor
    ~Matrix(){};

    // Copies matrix from RHS to LHS
    Matrix<T>& operator=(const Matrix<T> &RHS);

    // Access component (row,col)
    T& operator()(int row, int col);

    // Access component (row,col)
    const T& operator()(int row, int col) const;

    // Access component (row,col)
    T& operator()(int element);

    // Access component (row,col)
    const T& operator()(int element) const;

    T* getPointer(int element)
    {
        return &Data[element];
    }

    std::vector<T> getData() const
    {
        return Data;
    }

    // Gets the row of the matrix
    std::vector<T> getRow(int row) const;

    // Gets the row of the matrix
    void setRow(const std::vector<T> &rowData, int row);

    // Returns number of elements
    int size() const;

    // Returns number of rows
    int rows() const;

    // Returns number of columns
    int cols() const;

private:
    // Vector that contains the matrix data
    std::vector<T> Data;

    // Total number of elements
    int n;

    // Number of Rows
    int nRows;

    // Number of Columns
    int nCols;
};

template<typename T>
Matrix<T>::Matrix() : n(0), nRows(0), nCols(0) {}

template<typename T>
Matrix<T>::Matrix(const Matrix<T> &RHS) : Data(RHS.Data), n(RHS.n), nRows(RHS.nRows), nCols(RHS.nCols) {}

template<typename T>
Matrix<T>::Matrix(int rows, int cols) : n(rows * cols), nRows(rows), nCols(cols)
{
    Data = std::vector<T>(n);
    std::fill(Data.begin(), Data.end(), 0);
}

template<typename T>
Matrix<T>::Matrix(int rows, int cols, const T &scalar) : n(rows * cols), nRows(rows), nCols(cols)
{
    Data = std::vector<T>(n);
    std::fill(Data.begin(), Data.end(), scalar);
}

template<typename T>
Matrix<T>::Matrix(int rows, int cols, const std::vector<T> &InData) : n(rows * cols), nRows(rows), nCols(cols)
{
    assert(InData.size() == n);
    Data = std::vector<T>(InData);
}

template<typename T>
Matrix<T>& Matrix<T>::operator=(const Matrix<T> &RHS)
{
    n = RHS.n;
    nRows = RHS.nRows;
    nCols = RHS.nCols;
    Data = RHS.Data;
    return *this;
}

template<typename T>
inline const T& Matrix<T>::operator()(int row, int col) const
{
    assert( row < nRows );
    assert( col < nCols );
    return Data[row * nCols + col];
}

template<typename T>
inline T& Matrix<T>::operator()(int row, int col)
{
    assert(row < nRows);
    assert(col < nCols);
    return Data[row * nCols + col];
}

template<typename T>
inline const T& Matrix<T>::operator()(int element) const
{
    return Data[element];
}

template<typename T>
inline T& Matrix<T>::operator()(int element)
{
    return Data[element];
}

template <typename T>
std::vector<T> Matrix<T>::getRow(int row) const
{
    assert(row >= 0);
    assert(row < nRows);

    std::vector<T> tmp(nCols);
    for (int i = 0; i<nCols; ++i)
        tmp[i] = Data[row * nCols + i];

    return tmp;
}

template <typename T>
void Matrix<T>::setRow(const std::vector<T> &rowData, int row)
{
    assert(row >= 0);
    assert(row < nRows);
    assert(rowData.size() == nCols);

    for (int i = 0; i<nCols; ++i)
        Data[row * nCols + i] = rowData[i];
}

template<typename T>
inline int Matrix<T>::size() const
{
    return n;
}

template<typename T>
inline int Matrix<T>::rows() const
{
    return nRows;
}

template<typename T>
inline int Matrix<T>::cols() const
{
    return nCols;
}

#include "matrix-operations.h"
#endif // MATRIX_H
