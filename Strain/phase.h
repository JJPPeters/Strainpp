#ifndef PHASE_H
#define PHASE_H

#include <memory>
#include <complex>

#include "fftw3.h"

#include "matrix.h"
#include "utils.h"
#include "regression.h"
#include "coord.h"

#ifndef PI_H
#define PI_H
const double PI = 3.14159265358979323846;
#endif

class Phase
{
private:

    double _gxPx, _gyPx, _gx, _gy, _sigma;

    std::shared_ptr<Matrix<std::complex<double>>> _FFT;

    Matrix<double> _NormPhase;

    std::shared_ptr<fftw_plan> _FFTplan, _IFFTplan, _FFTdiffplan, _IFFTdiffplan;

public:

    Phase(std::shared_ptr<Matrix<std::complex<double>>> inputFFT, double gx, double gy, double sigma, std::shared_ptr<fftw_plan> forwardPlan, std::shared_ptr<fftw_plan> inversePlan);

    Matrix<double> getGaussianMask();

    Matrix<std::complex<double>> getMaskedFFT();

    Matrix<double> getBraggImage();

    Matrix<double> getHgImage();

    Matrix<double> getRawPhase();

    Matrix<double> getPhase();

    Matrix<double> getWrappedPhase();

    Coord2D<double> getGVector();

    void getDifferential(Matrix<std::complex<double>> &dx, Matrix<std::complex<double>> &dy);

    void refinePhase(int t, int l, int b, int r);


};

#endif // PHASE_H
