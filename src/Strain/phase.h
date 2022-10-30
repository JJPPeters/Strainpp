#ifndef PHASE_H
#define PHASE_H

#ifndef EIGEN_DEFAULT_TO_ROW_MAJOR
#define EIGEN_DEFAULT_TO_ROW_MAJOR
#endif

#ifndef EIGEN_INITIALIZE_MATRICES_BY_ZERO
#define EIGEN_INITIALIZE_MATRICES_BY_ZERO
#endif

#ifndef PI_H
#define PI_H
const double PI = 3.14159265358979323846;
#endif

#include <memory>
#include <complex>

#include "fftw3.h"

#include <Eigen/Dense>
#include "utils.h"
#include "coord.h"

class Phase
{
private:

    double _gxPx, _gyPx, _gx, _gy, _sigma;

    std::shared_ptr<Eigen::MatrixXcd> _FFT;

    Eigen::MatrixXd _NormPhase;

    std::shared_ptr<fftw_plan> _FFTplan, _IFFTplan, _FFTdiffplan, _IFFTdiffplan;

    double _angle;

public:

    Phase(std::shared_ptr<Eigen::MatrixXcd> inputFFT, double gx, double gy, double sigma, std::shared_ptr<fftw_plan> forwardPlan, std::shared_ptr<fftw_plan> inversePlan);

    void updateFFT(std::shared_ptr<Eigen::MatrixXcd> inputFFT)
    {
        _FFT = inputFFT;
    }

    Eigen::MatrixXd getGaussianMask();

    Eigen::MatrixXcd getMaskedFFT();

    Eigen::MatrixXd getBraggImage();

    Eigen::MatrixXd getRawPhase();

    Eigen::MatrixXd getPhase();

    Eigen::MatrixXd getWrappedPhase();

    Coord2D<double> getGVector();

    Coord2D<double> getGVectorPixels();

    void getDifferential(Eigen::MatrixXcd &dx, Eigen::MatrixXcd &dy, double angle);

    void getDifferential(Eigen::MatrixXcd &dx, Eigen::MatrixXcd &dy);

    void refinePhase(int t, int l, int b, int r);
};

#endif // PHASE_H
