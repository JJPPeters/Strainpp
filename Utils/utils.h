#ifndef UTILS_H
#define UTILS_H

#ifndef EIGEN_DEFAULT_TO_ROW_MAJOR
#define EIGEN_DEFAULT_TO_ROW_MAJOR
#endif

#ifndef EIGEN_INITIALIZE_MATRICES_BY_ZERO
#define EIGEN_INITIALIZE_MATRICES_BY_ZERO
#endif

#include <memory>
#include <cmath>
#include "fftw3.h"

#include <Eigen/Dense>

#ifndef PI_H
#define PI_H
const double PI = 3.14159265358979323846;
#endif

namespace Eigen
{
template <typename T>
using MatrixXT = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
}

namespace UtilsFFT {

    template <typename T>
    static Eigen::MatrixXT<T> preFFTShift(Eigen::MatrixXT<T> &input)
    {
        Eigen::MatrixXT<T> output(input.rows(), input.cols());

        #pragma omp parallel for
        for(int j = 0; j < input.rows(); ++j)
            for(int i =0; i < input.cols(); ++i)
                output(j, i) = std::pow(-1, i+j)*input(j, i);

        return output;
    }

    static void doFFTPlan(const std::shared_ptr<fftw_plan> plan, Eigen::MatrixXcd in, Eigen::MatrixXcd& out)
    {
        std::vector<std::complex<double>> buffer_in(in.size());
        std::vector<std::complex<double>> buffer_out(in.size());

        Eigen::Map<Eigen::MatrixXcd>(&buffer_in[0], in.rows(), in.cols()) = in;

        if (plan.get() != NULL)
            fftw_execute_dft((*plan), reinterpret_cast<fftw_complex*>(&buffer_in[0]), reinterpret_cast<fftw_complex*>(&buffer_out[0]));
        else
            return; //error throw?

        out = Eigen::Map<Eigen::MatrixXcd>(&buffer_out[0], in.rows(), in.cols());
    }

}

namespace UtilsMaths {

    static Eigen::MatrixXd MakeRotationMatrix(double angle)
    {
        Eigen::Matrix<double, 2, 2> rotmat;
        angle *= PI/180;

        rotmat(0, 0) = std::cos(angle);
        rotmat(0, 1) = std::sin(angle);
        rotmat(1, 0) = -1*std::sin(angle);
        rotmat(1, 1) = std::cos(angle);

        return rotmat;
    }

    static double Distance(int x1, int y1, int x2, int y2)
    {
        return std::sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
    }

    static double Distance(int x, int y)
    {
        return std::sqrt(x*x + y*y);
    }

    template <typename T>
    static Eigen::MatrixXT<T> HannWindow(const Eigen::MatrixXT<T> &input)
    {
        Eigen::MatrixXT<T> output(input.rows(), input.cols());

        std::vector<double> hann_y(input.rows());
        std::vector<double> hann_x(input.cols());

        #pragma omp parallel for
        for(int i = 0; i < input.rows(); ++i)
            hann_y[i] = 0.5 * (1 - std::cos( (2*PI*i) / (input.rows()-1) ));

        #pragma omp parallel for
        for(int i = 0; i < input.cols(); ++i)
            hann_x[i] = 0.5 * (1 - std::cos( (2*PI*i) / (input.cols()-1) ));

        #pragma omp parallel for
        for(int i = 0; i < input.rows(); ++i)
            for(int j = 0; j < input.cols(); ++j)
                output(i, j) = input(i, j) * hann_y[i] * hann_x[j];

        return output;
    }
}

#endif // UTILS_H

