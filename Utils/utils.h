#ifndef UTILS_H
#define UTILS_H

#include <memory>
#include <cmath>
#include "fftw3.h"

#include "matrix.h"

#ifndef PI_H
#define PI_H
const double PI = 3.14159265358979323846;
#endif

namespace UtilsFFT {

    template <typename T>
    static Matrix<T> preFFTShift(Matrix<T> &input)
    {
        Matrix<T> output(input.rows(), input.cols());

        for(int j = 0; j < input.rows(); ++j)
            for(int i =0; i < input.cols(); ++i)
                output(j, i) = std::pow(-1, i+j)*input(j, i);

        return output;
    }

    static void doFFTPlan(const std::shared_ptr<fftw_plan> plan, Matrix<std::complex<double> > in, Matrix<std::complex<double> >& out)
    {
        if (plan.get() != NULL)
            fftw_execute_dft((*plan), reinterpret_cast<fftw_complex*>(in.getPointer(0)), reinterpret_cast<fftw_complex*>(out.getPointer(0)));
        else
            return; //error throw?
    }

}

namespace UtilsMaths {

    static double Distance(int x1, int y1, int x2, int y2)
    {
        return std::sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
    }

    static double Distance(int x, int y)
    {
        return std::sqrt(x*x + y*y);
    }

    template <typename T>
    static Matrix<T> HannWindow(const Matrix<T> &input)
    {
        Matrix<T> output(input.rows(), input.cols());

        std::vector<double> hann_y(input.rows());
        std::vector<double> hann_x(input.cols());

        for(int i = 0; i < input.rows(); ++i)
            hann_y[i] = 0.5 * (1 - std::cos( (2*PI*i) / (input.rows()-1) ));

        for(int i = 0; i < input.cols(); ++i)
            hann_x[i] = 0.5 * (1 - std::cos( (2*PI*i) / (input.cols()-1) ));

        for(int i = 0; i < input.rows(); ++i)
            for(int j = 0; j < input.cols(); ++j)
                output(i, j) = input(i, j) * hann_y[i] * hann_x[j];

        return output;
    }
}


//void GPA::getLocalMaxima(const Matrix<double> &im, int &x, int &y, int r)
//{
//    // find values higher than

//    double max = std::numeric_limits<double>::max();
//    int xt = x;
//    int yt = y;
//    for (int j = yt-r ; j <= yt+r; ++j)
//        for (int i = xt-r ; i <= xt+r; ++i)
//        {
//            if(im(j, i) > max)
//            {
//                max = im(j, i);
//                x = i;
//                y = j;
//            }
//        }
//}

#endif // UTILS_H

