#include "phase.h"
#include "fftw3.h"

#include "iostream"

Phase::Phase(std::shared_ptr<Matrix<std::complex<double>>> inputFFT, double gx, double gy, double sigma, std::shared_ptr<fftw_plan> forwardPlan, std::shared_ptr<fftw_plan> inversePlan)
{
    _FFT = inputFFT;
    _gxPx = gx;
    _gyPx = gy;
    _gx = _gxPx / _FFT->cols();
    _gy = _gyPx / _FFT->rows();
    _sigma = sigma;

    _IFFTplan = inversePlan;
    _FFTplan = forwardPlan;

    _FFTdiffplan = std::make_shared<fftw_plan>(fftw_plan_dft_2d(_FFT->rows() + 2, _FFT->cols() + 2, NULL, NULL, FFTW_FORWARD, FFTW_ESTIMATE));
    _IFFTdiffplan = std::make_shared<fftw_plan>(fftw_plan_dft_2d(_FFT->rows() + 2, _FFT->cols() + 2, NULL, NULL, FFTW_BACKWARD, FFTW_ESTIMATE));

}

Matrix<double> Phase::getGaussianMask()
{
    // this justs shifts everything back to 0,0 at lower right corner
    double xf = _gxPx + _FFT->cols()/2;
    double yf = _gyPx + _FFT->rows()/2;

    Matrix<double> mask(_FFT->rows(), _FFT->cols());

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

Matrix<std::complex<double>> Phase::getMaskedFFT()
{
    Matrix<double> mask = getGaussianMask();
    Matrix<std::complex<double>> maskedFFT(mask.rows(), mask.cols());

    for (int i = 0; i < maskedFFT.size(); ++i)
        maskedFFT(i) = (*_FFT)(i) * mask(i);

    return maskedFFT;
}

Matrix<double> Phase::getBraggImage()
{
    Matrix<std::complex<double> > IFFT(_FFT->rows(), _FFT->cols());
    Matrix<double> braggImage(_FFT->rows(), _FFT->cols());

    // do IFFT of masked FFT then return abs or real part
    UtilsFFT::doFFTPlan(_IFFTplan, getMaskedFFT(), IFFT);

    IFFT = UtilsFFT::preFFTShift(IFFT);

    for(int i = 0; i < braggImage.size(); ++i)
        braggImage(i) = 2 * std::real(IFFT(i)) / (IFFT.rows() * IFFT.cols());

    return braggImage;
}

Matrix<double> Phase::getRawPhase()
{
    // only extracting phase so FFT normalising not needed
    Matrix<double> phase(_FFT->rows(), _FFT->cols());
    Matrix<std::complex<double> > IFFT(_FFT->rows(), _FFT->cols());

    UtilsFFT::doFFTPlan(_IFFTplan, getMaskedFFT(), IFFT);

    IFFT = UtilsFFT::preFFTShift(IFFT);

    for(int i = 0; i < phase.size(); ++i)
        phase(i) = std::arg(IFFT(i));

    return phase;
}

Matrix<double> Phase::getPhase()
{
    Matrix<double> phase = getRawPhase();

    for(int j = 0; j < phase.rows(); ++j)
        for(int i =0; i < phase.cols(); ++i)
        phase(j, i) = phase(j, i) - 2*PI * (i*_gx + j*_gy);

    return phase;
}

Matrix<double> Phase::getWrappedPhase()
{
    Matrix<double> phase = getPhase();

    for(int i =0; i < phase.size(); ++i)
        phase(i) = phase(i) - std::round(phase(i) / (2*PI)) * 2*PI;

    _NormPhase = phase;

    return _NormPhase;
}

void Phase::getDifferential(Matrix<std::complex<double>> &dx, Matrix<std::complex<double>> &dy)
{
    dx = Matrix<std::complex<double>>(_FFT->rows(), _FFT->cols());
    dy = Matrix<std::complex<double>>(_FFT->rows(), _FFT->cols());
    // contains the convolutino kernel, then the resultant differential
    Matrix<std::complex<double>> dx_kernel(_FFT->rows()+2, _FFT->cols()+2);
    Matrix<std::complex<double>> dy_kernel(_FFT->rows()+2, _FFT->cols()+2);
    // contains exponential form of strain
    Matrix<std::complex<double>> expPhase(_FFT->rows()+2, _FFT->cols()+2);
    // temp matrices to hold FFTs
    Matrix<std::complex<double>> phaseTemp(_FFT->rows()+2, _FFT->cols()+2);
    Matrix<std::complex<double>> xTemp(_FFT->rows()+2, _FFT->cols()+2);
    Matrix<std::complex<double>> yTemp(_FFT->rows()+2, _FFT->cols()+2);

    // fill kernels with pre fft shifted data
    for (int i = 0; i < 3; ++i)
    {
        // should all be divided by 6?
        dx_kernel(i, 0) = -1;
        dx_kernel(i, 2) = 1;

        dy_kernel(0, i) = -1;
        dy_kernel(2, i) = 1;
    }

    // create padded exponential phase matrix
    std::complex<double> im(0, 1);
    for (int i = 0; i < _NormPhase.rows(); ++i)
        for (int j = 0; j < _NormPhase.cols(); ++j)
            expPhase(i, j) = std::exp(im * _NormPhase(i, j));

    UtilsFFT::doFFTPlan(_FFTdiffplan, expPhase, phaseTemp);
    UtilsFFT::doFFTPlan(_FFTdiffplan, dx_kernel, xTemp);
    UtilsFFT::doFFTPlan(_FFTdiffplan, dy_kernel, yTemp);

    // do convolution
    for (int i = 0; i < phaseTemp.size(); ++i)
    {
        xTemp(i) = xTemp(i) * phaseTemp(i);
        yTemp(i) = yTemp(i) * phaseTemp(i);
    }

    UtilsFFT::doFFTPlan(_IFFTdiffplan, xTemp, dx_kernel);
    UtilsFFT::doFFTPlan(_IFFTdiffplan, yTemp, dy_kernel);

    // for normalising IFFT
    double nn = (_FFT->rows()+2)*(_FFT->cols()+2);

    for (int i = 1; i < expPhase.rows()-1; ++i)
        for (int j = 1; j < expPhase.cols()-1; ++j)
        {
            std::complex<double> ph = std::conj(expPhase(i, j));
            // TODO: test this is correct
            // Completely untexted but the 6 here is for the added value from the kernel (in pyhton version was divided at kernel creation)
            // from basic comparison to my python code it seems to give roughly the same values (there is some rotation though)
            dx(i-1, j-1) = std::imag(ph * dx_kernel(i, j) / (nn*6));
            dy(i-1, j-1) = std::imag(ph * dy_kernel(i, j) / (nn*6));
        }
}

Coord2D<double> Phase::getGVector()
{
    return Coord2D<double>(_gx, _gy);
}

void Phase::refinePhase(int t, int l, int b, int r)
{
    Matrix<double> area(t-b, r-l);

    for (int j = 0; j < t-b; ++j)
        for (int i = 0; i < r-l; ++i)
            area(j, i) = _NormPhase(j+b, i+l);

    std::vector<double> C;
    std::vector<double> W(area.size(), 1);
    Matrix<double> X(3, area.size());
    for (int j = 0; j < area.rows(); ++j)
        for (int i = 0; i < area.cols(); ++i)
        {
            X(0, j + i*area.rows()) = 1;
            X(1, j + i*area.rows()) = i;
            X(2, j + i*area.rows()) = j;
        }

    auto y = area.getData();

    Regression::LinearRegression(y, X, W, C);

    double dGxPx = C[1] / (2*PI) * area.cols();
    double dGyPx = C[2] / (2*PI) * area.rows();

    _gxPx += dGxPx;
    _gyPx += dGyPx;
    _gx = _gxPx/_FFT->cols();
    _gy = _gyPx/_FFT->rows();
}
