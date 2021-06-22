#include "gpa.h"
#include <iostream>

GPA::GPA(Eigen::MatrixXcd img)
{    
    // initialise vectors
    _Phases.resize(2);
    _Image = std::make_shared<Eigen::MatrixXcd>(img);
    _FFT = std::make_shared<Eigen::MatrixXcd>(Eigen::MatrixXcd(_Image->rows(), _Image->cols()));

    // do the FFT now
    UtilsFFT::doForwardFFT(_FFTplan, UtilsFFT::preFFTShift(*_Image), *_FFT);
}

std::shared_ptr<Eigen::MatrixXcd> GPA::getImage()
{
    return _Image;
}

std::shared_ptr<Eigen::MatrixXcd> GPA::getFFT()
{
    return _FFT;
}

std::shared_ptr<Eigen::MatrixXd> GPA::getExx()
{
    return _Exx;
}

std::shared_ptr<Eigen::MatrixXd> GPA::getExy()
{
    return _Exy;
}

std::shared_ptr<Eigen::MatrixXd> GPA::getEyx()
{
    return _Eyx;
}

std::shared_ptr<Eigen::MatrixXd> GPA::getEyy()
{
    return _Eyy;
}

int GPA::getGVectors()
{
    int xs = static_cast<int>(_FFT->cols());
    int ys = static_cast<int>(_FFT->rows());

    //Eigen::MatrixXd _PS(ys, xs);

    Eigen::MatrixXcd _PS(ys, xs);
    Eigen::MatrixXcd hanned = UtilsMaths::HannWindow(*_Image);
    UtilsFFT::doForwardFFT(_FFTplan, UtilsFFT::preFFTShift(hanned), _PS);
            //= UtilsMaths::HannWindow(original_image);;

    // get power spectrum as this will be used quite a bit
    #pragma omp parallel for
    for (int xIndex = 0; xIndex < xs; ++xIndex)
      for (int yIndex = 0; yIndex < ys; ++yIndex)
          _PS(yIndex, xIndex) =  std::log10(1+std::abs( _PS(yIndex, xIndex) ));

    // convert back to matrix coordinates
    int x0 = xs/2;
    int y0 = ys/2;

    double max = std::real(_PS(y0, x0));

    std::vector<double> averages;
    std::vector<double> vals;

    // loop through radii and sum
    // can't use openmp due to use of push_back ???
    for (int r = 1; r < std::min(xs, ys) / 4 - 1; ++r)
    {
        vals.clear();

        //get values within band
        for (int j = 0; j <= y0+r; ++j)
            for (int i = 0; i <= x0+r; ++i)
            {
                double dist = UtilsMaths::Distance(x0, y0, i, j);
                if (dist < r+1 && dist > r-1)
                    vals.push_back(std::real(_PS(j, i)));
            }

        std::sort(vals.begin(), vals.end(), std::greater<>());

        double av = 0;
        int sz = std::min((int)vals.size(), 10);

        for (int i = 0; i < sz; ++i)
            av += vals[i];
        av /= (sz * max);

        averages.push_back(av);
    }

    if(averages.empty())
        return std::min(xs, ys) / 2;

    // normalise
    auto minIterator = std::min_element(averages.begin(), averages.end());
    double min = *minIterator;
    for (double &average : averages)
        average -= min;
    auto maxIterator = std::max_element(averages.begin(), averages.end());
    max = *maxIterator;
    for (double &average : averages)
        average /= max;

    int inner = 0;

    // can't use openmp due to break statement
    // need to decide what's that fastest way of doing this
    for(int i = 0; i < averages.size(); ++i)
    {
        if (averages[i] < 0.9 * max)
        {
            inner = i;
            break;
        }
        averages[i] *= 0;
    }

    #pragma omp parallel for
    for(int i = 0; i < averages.size(); ++i)
    {
        averages[i] *= (averages.size() - i);
    }

    // just finds the maximum value not in the centre.
    int radius = static_cast<int>(std::distance(averages.begin(), std::max_element(averages.begin() + inner, averages.end())));

    return radius;
}


void GPA::calculatePhase(int i, double gx, double gy, double sig)
{
    //if (i != 0 && i != 1)
        //throw

    _Phases[i] = std::make_shared<Phase>(Phase(_FFT, gx, gy, sig, _FFTplan, _IFFTplan));
}

std::shared_ptr<Phase> GPA::getPhase(int i)
{
    //if (i != 0 && i != 1)
        //throw

    return _Phases[i];
}

void GPA::calculateDistortion(double angle, std::string mode)
{
    Eigen::MatrixXcd d1dx, d1dy, d2dx, d2dy;

    // get differential (rotated to coodinate system)
    _Phases[0]->getDifferential(d1dx, d1dy, angle);
    _Phases[1]->getDifferential(d2dx, d2dy, angle);

    //calculate A matrix (from G matrix)
    //here I do several steps in one go, but all I am doing is Inverse(Transpose(G)) = A
    Eigen::Matrix<double, 2, 2> A;
    Coord2D<double> g1 = _Phases[0]->getGVector();
    Coord2D<double> g2 = _Phases[1]->getGVector();

    //NOTE: I've created the A matrix already transposed
    A << g1.x, g1.y, g2.x, g2.y;
    A = A.inverse().eval();
    // rotate the basis
    A = UtilsMaths::MakeRotationMatrix(angle) * A;

    double factor = -1.0 / (2.0 * PI);

    // not sure if I get anything out of this really, all the tasks use the same variables
#pragma omp parallel
    {
    #pragma omp single
        {
        #pragma omp task
            _Exx = std::make_shared<Eigen::MatrixXd>((factor * (A(0, 0) * d1dx + A(0, 1) * d2dx)).real());
        #pragma omp task
            _Exy = std::make_shared<Eigen::MatrixXd>((factor * (A(0, 0) * d1dy + A(0, 1) * d2dy)).real());
        #pragma omp task
            _Eyx = std::make_shared<Eigen::MatrixXd>((factor * (A(1, 0) * d1dx + A(1, 1) * d2dx)).real());
        #pragma omp task
            _Eyy = std::make_shared<Eigen::MatrixXd>((factor * (A(1, 0) * d1dy + A(1, 1) * d2dy)).real());
        }
    }

    if (mode == "Strain")
    {
        _Exy = std::make_shared<Eigen::MatrixXd>(0.5 * (*_Exy + *_Eyx));
        _Eyx = std::make_shared<Eigen::MatrixXd>(*_Exy);
    }
    else if (mode == "Rotation")
    {
        auto temp = *_Exy;
        _Exx = std::make_shared<Eigen::MatrixXd>( Eigen::MatrixXd::Constant(_Exx->rows(), _Exx->cols(), 0.0) );
        _Exy = std::make_shared<Eigen::MatrixXd>(0.5 * (*_Exy - *_Eyx));
        _Eyx = std::make_shared<Eigen::MatrixXd>(0.5 * (*_Eyx - temp));
        _Eyy = std::make_shared<Eigen::MatrixXd>( Eigen::MatrixXd::Constant(_Eyy->rows(), _Eyy->cols(), 0.0) );
    }
    else if (mode == "Dilitation")
    {
        _Exx = std::make_shared<Eigen::MatrixXd>(*_Exx + *_Eyy);
        _Exy = std::make_shared<Eigen::MatrixXd>( Eigen::MatrixXd::Constant(_Exx->rows(), _Exx->cols(), 0.0) );
        _Eyx = std::make_shared<Eigen::MatrixXd>( Eigen::MatrixXd::Constant(_Exx->rows(), _Exx->cols(), 0.0) );
        _Eyy = std::make_shared<Eigen::MatrixXd>( Eigen::MatrixXd::Constant(_Eyy->rows(), _Eyy->cols(), 0.0) );
    }
}
