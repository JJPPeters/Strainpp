#ifndef RANSAC_H
#define RANSAC_H

#include "matrix.h"
#include "fftw3.h"
#include <complex>
#include <memory>
#include <cmath>
#include <limits>
#include <algorithm>

class RANSAC
{
public:
    RANSAC(std::shared_ptr<Matrix<std::complex<double>>> FFT, std::shared_ptr<fftw_plan> FFTplan, std::shared_ptr<fftw_plan> IFFTplan);

    //Finds the g-vectors from the constructed power spectrum. coordinates of initial point are given wrt to center of the image.
    int FindVectors(int xPos, int yPos);

private:
    Matrix<double> _PS;

    // Finds the maxima from a square with sides (2*r+1) centered on (x, y)
    // x, y are modified in place to the new maxima
    double getLocalMaxima(int &x, int &y, int r);

    double _distance(int x1, int y1, int x2, int y2)
    {
        return std::sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
    }

    double _distance(int x, int y)
    {
        return std::sqrt(x*x + y*y);
    }

};

#endif // RANSAC_H
