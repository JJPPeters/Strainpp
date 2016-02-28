#ifndef GPA_H
#define GPA_H

#include <memory>
#include <complex>
#include <algorithm>
#include <functional>

#include "fftw3.h"

#include "matrix.h"
#include "utils.h"
#include "phase.h"
#include "coord.h"

#ifndef PI_H
#define PI_H
const double PI = 3.14159265358979323846;
#endif

class GPA
{
private:
    std::shared_ptr<Matrix<std::complex<double>>> _Image, _FFT;
    
    std::shared_ptr<Matrix<double>> _Exx, _Exy, _Eyx, _Eyy;

    std::vector<std::shared_ptr<Phase>> _Phases;

    std::shared_ptr<fftw_plan> _FFTplan, _IFFTplan;

    Matrix<double> _GetRotationMatrix(double angle);

public:

    GPA(Matrix<std::complex<double>> img);

    void updateImage(Matrix<std::complex<double>> img)
    {
        if (img.rows() != _Image->rows() || img.cols() != _Image->cols())
            return;

        _Image = std::make_shared<Matrix<std::complex<double>>>(img);
        UtilsFFT::doFFTPlan(_FFTplan, UtilsFFT::preFFTShift(*_Image), *_FFT);
    }

    std::shared_ptr<Matrix<std::complex<double>>> getImage();

    std::shared_ptr<Matrix<std::complex<double>>> getFFT();

    std::shared_ptr<Matrix<double>> getExx();

    std::shared_ptr<Matrix<double>> getExy();

    std::shared_ptr<Matrix<double>> getEyx();

    std::shared_ptr<Matrix<double>> getEyy();

    int getGVectors();

    void calculatePhase(int i, double gx, double gy, double sig);

    std::shared_ptr<Phase> getPhase(int i);

    void rotateReference();

    void calculateStrain(double angle);

    Coord2D<int> getSize()
    {
        return Coord2D<int>(_Image->cols(), _Image->rows());
    }

};

#endif // GPA_H
