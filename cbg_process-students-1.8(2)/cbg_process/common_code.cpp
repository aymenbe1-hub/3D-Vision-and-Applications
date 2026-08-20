#include "common_code.hpp"
#include <opencv2/imgproc/imgproc.hpp>

cv::Mat
fsiv_convert_image_byte_to_float(const cv::Mat &img)
{
    CV_Assert(img.depth() == CV_8U);
    cv::Mat out;
    
    // Convertimos de [0, 255] (CV_8U) a [0.0, 1.0] (CV_32F) multiplicando por 1/255.0
    img.convertTo(out, CV_32F, 1.0 / 255.0);

    CV_Assert(out.rows == img.rows && out.cols == img.cols);
    CV_Assert(out.depth() == CV_32F);
    CV_Assert(img.channels() == out.channels());
    return out;
}

cv::Mat
fsiv_convert_image_float_to_byte(const cv::Mat &img)
{
    CV_Assert(img.depth() == CV_32F);
    cv::Mat out;
    
    // Convertimos de [0.0, 1.0] (CV_32F) a [0, 255] (CV_8U) multiplicando por 255.0
    img.convertTo(out, CV_8U, 255.0);

    CV_Assert(out.rows == img.rows && out.cols == img.cols);
    CV_Assert(out.depth() == CV_8U);
    CV_Assert(img.channels() == out.channels());
    return out;
}

cv::Mat
fsiv_convert_bgr_to_hsv(const cv::Mat &img)
{
    CV_Assert(img.channels() == 3);
    cv::Mat out;
    
    // Conversión del espacio de color BGR a HSV
    cv::cvtColor(img, out, cv::COLOR_BGR2HSV);

    CV_Assert(out.channels() == 3);
    return out;
}

cv::Mat
fsiv_convert_hsv_to_bgr(const cv::Mat &img)
{
    CV_Assert(img.channels() == 3);
    cv::Mat out;
    
    // Conversión del espacio de color HSV de vuelta a BGR
    cv::cvtColor(img, out, cv::COLOR_HSV2BGR);

    CV_Assert(out.channels() == 3);
    return out;
}

cv::Mat
fsiv_cbg_process(const cv::Mat &in,
                 double contrast, double brightness, double gamma,
                 bool only_luma)
{
    CV_Assert(in.depth() == CV_8U);
    cv::Mat out;
    
    // 1. Convertir la imagen de entrada a rango flotante [0.0, 1.0]
    cv::Mat float_img = fsiv_convert_image_byte_to_float(in);
    cv::Mat processed_float;

    // 2. Procesar según los canales y la opción "only_luma"
    if (in.channels() == 3 && only_luma)
    {
        // Pasamos a espacio de color HSV
        cv::Mat hsv = fsiv_convert_bgr_to_hsv(float_img);
        
        // Desentrelazamos los 3 canales (H, S, V)
        std::vector<cv::Mat> channels;
        cv::split(hsv, channels);
        
        // Aplicamos la ecuación: I' = c * I^g + b únicamente sobre el canal V (Luminancia / Luma)
        cv::pow(channels[2], gamma, channels[2]);
        channels[2] = contrast * channels[2] + brightness;
        
        // Volvemos a entrelazar los canales modificados y convertimos a BGR
        cv::merge(channels, hsv);
        processed_float = fsiv_convert_hsv_to_bgr(hsv);
    }
    else
    {
        // Procesado completo ("Full" RGB o Monocroma) en todos sus canales
        cv::pow(float_img, gamma, processed_float);
        processed_float = contrast * processed_float + brightness;
    }

    // 3. Devolver la imagen al tipo byte [0, 255] original
    out = fsiv_convert_image_float_to_byte(processed_float);

    CV_Assert(out.rows == in.rows && out.cols == in.cols);
    CV_Assert(out.depth() == CV_8U);
    CV_Assert(out.channels() == in.channels());
    return out;
}