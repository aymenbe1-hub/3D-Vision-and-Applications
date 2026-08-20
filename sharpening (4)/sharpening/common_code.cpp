#include <iostream>
#include "common_code.hpp"
#include <opencv2/imgproc.hpp>

cv::Mat
fsiv_create_gaussian_filter(const int r)
{
    CV_Assert(r > 0);
    cv::Mat ret_v;
    
    // 1. Obtener el kernel unidimensional (vector columna) con el radio r (tamaño = 2*r + 1)
    cv::Mat kernel_1d = cv::getGaussianKernel(2 * r + 1, -1.0, CV_32FC1);
    
    // 2. Crear el filtro bidimensional mediante el producto externo (kernel_1d * kernel_1d^T)
    ret_v = kernel_1d * kernel_1d.t();
    
    CV_Assert(ret_v.type() == CV_32FC1);
    CV_Assert(ret_v.rows == (2 * r + 1) && ret_v.rows == ret_v.cols);
    CV_Assert(std::abs(1.0 - cv::sum(ret_v)[0]) < 1.0e-6);
    return ret_v;
}

cv::Mat fsiv_fill_expansion(cv::Mat const &in, const int r)
{
    CV_Assert(!in.empty());
    CV_Assert(r > 0);
    cv::Mat ret_v;
    
    // Expandir la imagen rellenando los nuevos bordes con ceros constantes (BORDER_CONSTANT, value=0)
    cv::copyMakeBorder(in, ret_v, r, r, r, r, cv::BORDER_CONSTANT, cv::Scalar(0));
    
    CV_Assert(ret_v.type() == in.type());
    CV_Assert(ret_v.rows == in.rows + 2 * r);
    CV_Assert(ret_v.cols == in.cols + 2 * r);
    return ret_v;
}

cv::Mat fsiv_circular_expansion(cv::Mat const &in, const int r)
{
    CV_Assert(!in.empty());
    CV_Assert(r > 0);
    cv::Mat ret_v;
    
    // Expandir la imagen rellenando los bordes de forma cíclica/circular (BORDER_WRAP)
    cv::copyMakeBorder(in, ret_v, r, r, r, r, cv::BORDER_WRAP);
    
    CV_Assert(ret_v.type() == in.type());
    CV_Assert(ret_v.rows == in.rows + 2 * r);
    CV_Assert(ret_v.cols == in.cols + 2 * r);
    return ret_v;
}

cv::Mat fsiv_create_lap4_filter()
{
    cv::Mat ret_v;
    
    // Ajustado con los signos correctos para el test: centro negativo y vecinos positivos
    ret_v = (cv::Mat_<float>(3, 3) << 
              0,  1,  0,
              1, -4,  1,
              0,  1,  0);
             
    CV_Assert(!ret_v.empty());
    CV_Assert(ret_v.rows == 3 && ret_v.cols == 3);
    CV_Assert(ret_v.type() == CV_32FC1);
    return ret_v;
}

cv::Mat fsiv_create_lap8_filter()
{
    cv::Mat ret_v;
    
    // Ajustado con los signos correctos para el test: centro negativo y vecinos positivos
    ret_v = (cv::Mat_<float>(3, 3) << 
              1,  1,  1,
              1, -8,  1,
              1,  1,  1);
             
    CV_Assert(!ret_v.empty());
    CV_Assert(ret_v.rows == 3 && ret_v.cols == 3);
    CV_Assert(ret_v.type() == CV_32FC1);
    return ret_v;
}

cv::Mat fsiv_create_dog_filter(int r1, int r2)
{
    CV_Assert(r1 > 0 && r1 < r2);
    cv::Mat ret_v;
    
    // DoG = G_sigma2 - G_sigma1. Se expande G_sigma1 rellenándolo con ceros 
    // hasta alcanzar el tamaño de G_sigma2.
    cv::Mat g1 = fsiv_create_gaussian_filter(r1);
    cv::Mat g2 = fsiv_create_gaussian_filter(r2);
    
    cv::Mat g1_expanded;
    int pad = r2 - r1;
    cv::copyMakeBorder(g1, g1_expanded, pad, pad, pad, pad, cv::BORDER_CONSTANT, cv::Scalar(0));
    
    // Restamos las dos gaussianas
    ret_v = g2 - g1_expanded;
    
    CV_Assert(ret_v.type() == CV_32FC1);
    CV_Assert(ret_v.rows == (2 * r2 + 1) && ret_v.rows == ret_v.cols);
    return ret_v;
}

cv::Mat
fsiv_create_sharpening_filter(const int filter_type, int r1, int r2)
{
    CV_Assert(0 <= filter_type && filter_type <= 2);
    CV_Assert(filter_type != 2 || (0 < r1 && r1 < r2));
    cv::Mat filter;
    
    if (filter_type == 0) // LAP_4
    {
        cv::Mat lap = fsiv_create_lap4_filter();
        cv::Mat delta = cv::Mat::zeros(3, 3, CV_32FC1);
        delta.at<float>(1, 1) = 1.0f;
        filter = delta - lap; 
    }
    else if (filter_type == 1) // LAP_8
    {
        cv::Mat lap = fsiv_create_lap8_filter();
        cv::Mat delta = cv::Mat::zeros(3, 3, CV_32FC1);
        delta.at<float>(1, 1) = 1.0f;
        filter = delta - lap; 
    }
    else if (filter_type == 2) // DOG
    {
        cv::Mat dog = fsiv_create_dog_filter(r1, r2);
        cv::Mat delta = cv::Mat::zeros(2 * r2 + 1, 2 * r2 + 1, CV_32FC1);
        delta.at<float>(r2, r2) = 1.0f;
        filter = delta - dog;
    }
    
    CV_Assert(!filter.empty() && filter.type() == CV_32FC1);
    CV_Assert((filter_type == 2) || (filter.rows == 3 && filter.cols == 3));
    CV_Assert((filter_type != 2) || (filter.rows == (2 * r2 + 1) &&
                                     filter.cols == (2 * r2 + 1)));
    return filter;
}

cv::Mat
fsiv_image_sharpening(const cv::Mat &in, int filter_type,
                      int r1, int r2, bool circular)
{
    CV_Assert(in.type() == CV_32FC1);
    CV_Assert(0 < r1 && r1 < r2);
    CV_Assert(0 <= filter_type && filter_type <= 2);
    cv::Mat out;

    // 1. Determinar el radio del filtro para expandir correctamente la imagen
    int r = (filter_type == 2) ? r2 : 1;
    
    // 2. Expandir la imagen de entrada según el tipo seleccionado (con ceros o circular)
    cv::Mat extended_in;
    if (circular)
        extended_in = fsiv_circular_expansion(in, r);
    else
        extended_in = fsiv_fill_expansion(in, r);
        
    // 3. Crear el filtro de enfoque correspondiente
    cv::Mat sharpening_filter = fsiv_create_sharpening_filter(filter_type, r1, r2);
    
    // 4. Realizar la convolución sobre la imagen expandida utilizando BORDER_ISOLATED
    cv::Mat extended_out;
    cv::filter2D(extended_in, extended_out, CV_32FC1, sharpening_filter, cv::Point(-1, -1), 0.0, cv::BORDER_ISOLATED);
    
    // 5. Recortar la región central de tamaño original
    out = extended_out(cv::Rect(r, r, in.cols, in.rows)).clone();

    CV_Assert(out.type() == in.type());
    CV_Assert(out.size() == in.size());
    return out;
}