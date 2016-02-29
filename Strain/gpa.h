#ifndef GPA_H
#define GPA_H

#ifndef PI_H
#define PI_H
const double PI = 3.14159265358979323846;
#endif

#ifndef EIGEN_DEFAULT_TO_ROW_MAJOR
#define EIGEN_DEFAULT_TO_ROW_MAJOR
#endif

#include <memory>
#include <complex>
#include <algorithm>
#include <functional>

#include "fftw3.h"

#include <Eigen/Dense>
#include "utils.h"
#include "phase.h"
#include "coord.h"

class GPA
{
private:
    std::shared_ptr<Eigen::MatrixXcd> _Image, _FFT;
    
    std::shared_ptr<Eigen::MatrixXd> _Exx, _Exy, _Eyx, _Eyy;

    std::vector<std::shared_ptr<Phase>> _Phases;

    std::shared_ptr<fftw_plan> _FFTplan, _IFFTplan;

    Eigen::MatrixXd _GetRotationMatrix(double angle);

public:

    GPA(Eigen::MatrixXcd img);

    void updateImage(Eigen::MatrixXcd img)
    {
        if (img.rows() != _Image->rows() || img.cols() != _Image->cols())
            return;

        _Image = std::make_shared<Eigen::MatrixXcd>(img);
        UtilsFFT::doFFTPlan(_FFTplan, UtilsFFT::preFFTShift(*_Image), *_FFT);
    }

    std::shared_ptr<Eigen::MatrixXcd> getImage();

    std::shared_ptr<Eigen::MatrixXcd> getFFT();

    std::shared_ptr<Eigen::MatrixXd> getExx();

    std::shared_ptr<Eigen::MatrixXd> getExy();

    std::shared_ptr<Eigen::MatrixXd> getEyx();

    std::shared_ptr<Eigen::MatrixXd> getEyy();

    int getGVectors();

    void calculatePhase(int i, double gx, double gy, double sig);

    std::shared_ptr<Phase> getPhase(int i);

    void rotateReference();

    void calculateStrain(double angle);

    void correctStrains();

    Coord2D<int> getSize()
    {
        return Coord2D<int>(_Image->cols(), _Image->rows());
    }

};

#endif // GPA_H
