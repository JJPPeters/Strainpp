#include "phase.h"

#include "iostream"

Phase::Phase(std::shared_ptr<Eigen::MatrixXcd> inputFFT, double gx, double gy, double sigma, std::shared_ptr<fftw_plan> forwardPlan, std::shared_ptr<fftw_plan> inversePlan)
{
    _angle = 0;
    _FFT = std::move(inputFFT);
    _gxPx = gx;
    _gyPx = gy;
    _gx = _gxPx / _FFT->cols();
    _gy = _gyPx / _FFT->rows();
    _sigma = sigma;

    _IFFTplan = std::move(inversePlan);
    _FFTplan = std::move(forwardPlan);

//    _FFTdiffplan = std::make_shared<fftw_plan>(fftw_plan_dft_2d(static_cast<int>(_FFT->rows() + 2),
//                                                                static_cast<int>(_FFT->cols() + 2), NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE));
//    _IFFTdiffplan = std::make_shared<fftw_plan>(fftw_plan_dft_2d(static_cast<int>(_FFT->rows() + 2),
//                                                                 static_cast<int>(_FFT->cols() + 2), NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE));

}

Eigen::MatrixXd Phase::getGaussianMask()
{
    // this just shifts everything back to 0,0 at lower right corner
    double xf = _gxPx + _FFT->cols()/2;
    double yf = _gyPx + _FFT->rows()/2;

    Eigen::MatrixXd mask(_FFT->rows(), _FFT->cols());

    #pragma omp parallel for
    for (int j = 0; j < _FFT->rows(); ++j)
    {
        double yc = (double)j - yf;
        for (int i = 0; i < _FFT->cols(); ++i)
        {
            double xc = (double)i - xf;
            mask(j*mask.cols() + i) = std::exp( -0.5 * (xc*xc + yc*yc) / (_sigma*_sigma) );
        }
    }

    return mask;
}

Eigen::MatrixXcd Phase::getMaskedFFT()
{
    Eigen::MatrixXd mask = getGaussianMask();
    //return (*_FFT).cwiseProduct(mask);

    Eigen::MatrixXcd maskedFFT(mask.rows(), mask.cols());

    maskedFFT = _FFT->cwiseProduct(mask);

    return maskedFFT;
}

Eigen::MatrixXd Phase::getBraggImage()
{
    Eigen::MatrixXcd IFFT(_FFT->rows(), _FFT->cols());

    // do IFFT of masked FFT then return abs or real part
    UtilsFFT::doBackwardFFT(_IFFTplan, getMaskedFFT(), IFFT);

    IFFT = UtilsFFT::preFFTShift(IFFT);

    return 2 * IFFT.real() / (IFFT.rows() * IFFT.cols());
}

Eigen::MatrixXd Phase::getRawPhase()
{
    // only extracting phase so FFT normalising not needed
    Eigen::MatrixXd phase(_FFT->rows(), _FFT->cols());
    Eigen::MatrixXcd IFFT(_FFT->rows(), _FFT->cols());

    UtilsFFT::doBackwardFFT(_IFFTplan, getMaskedFFT(), IFFT);

    IFFT = UtilsFFT::preFFTShift(IFFT);

    // don't think eigen has a bette version of this
    for(int i = 0; i < phase.size(); ++i)
        phase(i) = std::arg(IFFT(i));

    return phase;
}

Eigen::MatrixXd Phase::getPhase()
{
    Eigen::MatrixXd phase = getRawPhase();

    // can this be made faster in eigen?
    #pragma omp parallel for
    for(int j = 0; j < phase.rows(); ++j)
        for(int i =0; i < phase.cols(); ++i)
            phase(j, i) = phase(j, i) - 2*PI * (i*_gx + j*_gy);

    return phase;
}

Eigen::MatrixXd Phase::getWrappedPhase()
{
    Eigen::MatrixXd phase = getPhase();

    #pragma omp parallel for
    for(int i = 0; i < phase.size(); ++i)
        phase(i) -= std::round(phase(i) / (2*PI)) * 2*PI;

    _NormPhase = phase;

    return _NormPhase;
}

void Phase::getDifferential(Eigen::MatrixXcd &dx, Eigen::MatrixXcd &dy, double angle)
{
    _angle = angle;
    getDifferential(dx, dy);
}

void Phase::getDifferential(Eigen::MatrixXcd &dx, Eigen::MatrixXcd &dy)
{
    dx = Eigen::MatrixXcd(_FFT->rows(), _FFT->cols());
    dy = Eigen::MatrixXcd(_FFT->rows(), _FFT->cols());
    // contains the convolution kernel, then the resultant differential
    Eigen::MatrixXcd dx_kernel(_FFT->rows()+2, _FFT->cols()+2);
    Eigen::MatrixXcd dy_kernel(_FFT->rows()+2, _FFT->cols()+2);
    // contains exponential form of strain
    Eigen::MatrixXcd expPhase(_FFT->rows()+2, _FFT->cols()+2);
    // temp matrices to hold FFTs
    Eigen::MatrixXcd phaseTemp(_FFT->rows()+2, _FFT->cols()+2);
    Eigen::MatrixXcd xTemp(_FFT->rows()+2, _FFT->cols()+2);
    Eigen::MatrixXcd yTemp(_FFT->rows()+2, _FFT->cols()+2);

    // fill kernels with pre fft shifted data
    #pragma omp parallel for
    for (int i = 0; i < 3; ++i)
    {
        // should all be divided by 6 (this is now done later)

        dx_kernel(i, 0) = 1;
        dx_kernel(i, 2) = -1;
        // do not need to correct for image direction like I said before
        // that was stupid... (thanks Dr Benedykt R. Jany)
        dy_kernel(0, i) = 1;
        dy_kernel(2, i) = -1;
    }

    // create padded exponential phase matrix
    std::complex<double> im(0, 1);
    #pragma omp parallel for
    for (int i = 0; i < _NormPhase.rows(); ++i)
        for (int j = 0; j < _NormPhase.cols(); ++j)
            expPhase(i, j) = std::exp(im * _NormPhase(i, j));

//    #pragma omp parallel
//    {
//    #pragma omp single
//        {
//        #pragma omp task
            UtilsFFT::doForwardFFT(_FFTdiffplan, expPhase, phaseTemp);
//        #pragma omp task
            UtilsFFT::doForwardFFT(_FFTdiffplan, dx_kernel, xTemp);
//        #pragma omp task
            UtilsFFT::doForwardFFT(_FFTdiffplan, dy_kernel, yTemp);
//        }
//    }

    // do convolution
    #pragma omp parallel for
    for (int i = 0; i < phaseTemp.size(); ++i)
    {
        xTemp(i) = xTemp(i) * phaseTemp(i);
        yTemp(i) = yTemp(i) * phaseTemp(i);
    }

//    #pragma omp parallel
//    {
//    #pragma omp single
//        {
//        #pragma omp task
            UtilsFFT::doBackwardFFT(_IFFTdiffplan, xTemp, dx_kernel);
//        #pragma omp task
            UtilsFFT::doBackwardFFT(_IFFTdiffplan, yTemp, dy_kernel);
//        }
//    }

    // for normalising IFFT
    double nn = (_FFT->rows()+2)*(_FFT->cols()+2);

    #pragma omp parallel for
    for (int i = 1; i < expPhase.rows()-1; ++i)
        for (int j = 1; j < expPhase.cols()-1; ++j)
        {
            std::complex<double> ph = std::conj(expPhase(i, j));
            // TODO: test this is correct
            // Completely untested but the 6 here is for the added value from the kernel (in python version was divided at kernel creation)
            // from basic comparison to my python code it seems to give roughly the same values (there is some rotation though)
            dx(i-1, j-1) = std::imag(ph * dx_kernel(i, j) / (nn*6));
            dy(i-1, j-1) = std::imag(ph * dy_kernel(i, j) / (nn*6));
        }

    auto rotMat = UtilsMaths::MakeRotationMatrix(_angle);

    Eigen::MatrixXcd temp;
    temp = rotMat(0,0) * dx + rotMat(0,1) * dy;
    dy = rotMat(1,0) * dx + rotMat(1,1) * dy;
    dx = temp;

}

Coord2D<double> Phase::getGVector()
{
    return {_gx, _gy};
}

Coord2D<double> Phase::getGVectorPixels()
{
    return {_gxPx, _gyPx};
}

void Phase::refinePhase(int t, int l, int b, int r)
{
    // Here we use linear regression to find the gradient of the selected area,
    // We then readjust the G-vectors to flatten this gradient.
    Eigen::MatrixXd area(t-b, r-l);

    #pragma omp parallel for
    for (int j = 0; j < t-b; ++j)
        for (int i = 0; i < r-l; ++i)
            area(j, i) = _NormPhase(j+b, i+l);

    Eigen::MatrixXd X(area.size(), 3);

    #pragma omp parallel for
    for (int i = 0; i < area.cols(); ++i)
        for (int j = 0; j < area.rows(); ++j)
        {
            X(i + j*area.cols(), 0) = 1;
            X(i + j*area.cols(), 1) = i;
            X(i + j*area.cols(), 2) = j;
        }

    area.resize(area.size(), 1);

    // need rows == rows...
    Eigen::Vector3d C = X.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(area);

    double dGxPx = C[1] / (2*PI) * _FFT->cols();
    double dGyPx = C[2] / (2*PI) * _FFT->rows();

    _gxPx += dGxPx;
    _gyPx += dGyPx;
    _gx = _gxPx/_FFT->cols();
    _gy = _gyPx/_FFT->rows();
}
