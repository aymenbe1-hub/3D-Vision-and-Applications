#include "common_code.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>

cv::Mat fsiv_color_rescaling(const cv::Mat &in, const cv::Scalar &from, const cv::Scalar &to)
{
    cv::Mat out;
    
    // Calculamos el factor de escala de forma segura: factor = to / from
    cv::Scalar factor;
    for (int i = 0; i < 4; ++i)
    {
        factor[i] = (from[i] != 0.0) ? (to[i] / from[i]) : 0.0;
    }
    
    // Multiplicamos la matriz de entrada por el factor de escala canal a canal
    out = in.mul(factor);
    
    CV_Assert(out.type() == in.type());
    CV_Assert(out.size() == in.size());
    return out;
}

cv::Mat fsiv_gray_world_color_balance(cv::Mat const &in)
{
    CV_Assert(in.type() == CV_8UC3);
    cv::Mat out;
    
    // 1. Calculamos el valor promedio (B, G, R) de todos los píxeles de la imagen
    cv::Scalar mean_val = cv::mean(in);
    
    // 2. El color destino ideal en Gray World es el gris neutro (128, 128, 128)
    cv::Scalar target(128.0, 128.0, 128.0);
    
    // 3. Escalamos usando la función previa
    out = fsiv_color_rescaling(in, mean_val, target);
    
    CV_Assert(out.type() == in.type());
    CV_Assert(out.rows == in.rows && out.cols == in.cols);
    return out;
}

cv::Mat fsiv_convert_bgr_to_gray(const cv::Mat &img, cv::Mat &out)
{
    CV_Assert(img.channels() == 3);
    
    // Conversión convencional de BGR a escala de grises
    cv::cvtColor(img, out, cv::COLOR_BGR2GRAY);
    
    CV_Assert(out.channels() == 1);
    return out;
}

cv::Mat fsiv_compute_image_histogram(cv::Mat const &img)
{
    CV_Assert(img.type() == CV_8UC1);
    cv::Mat hist;
    
    // Configuración para el histograma de un canal monocromo (256 niveles)
    int histSize = 256;
    float range[] = { 0, 256 };
    const float* histRange = { range };
    
    cv::calcHist(&img, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange);
    
    CV_Assert(hist.type() == CV_32FC1);
    CV_Assert(hist.rows == 256 && hist.cols == 1);
    return hist;
}

int fsiv_compute_histogram_percentile(cv::Mat const &hist, float p_value)
{
    CV_Assert(hist.type() == CV_32FC1);
    CV_Assert(hist.cols == 1);
    CV_Assert(0.0 <= p_value && p_value <= 1.0);

    int p = 0;
    
    // 1. Calculamos el área total sumando todas las frecuencias del histograma
    double total_area = cv::sum(hist)[0];
    
    // 2. Definimos el valor de corte que buscamos alcanzar
    double target_sum = p_value * total_area;
    
    // 3. Buscamos el índice acumulativo más pequeño
    double current_sum = 0.0;
    for (p = 0; p < hist.rows; ++p)
    {
        current_sum += hist.at<float>(p);
        if (current_sum >= target_sum)
        {
            break;
        }
    }

    // Aseguramos que el índice se mantenga dentro de los límites válidos
    if (p >= hist.rows)
        p = hist.rows - 1;

    CV_Assert(0 <= p && p < hist.rows);
    return p;
}

cv::Mat fsiv_white_patch_color_balance(cv::Mat const &in, float p)
{
    CV_Assert(in.type() == CV_8UC3);
    CV_Assert(0.0f <= p && p < 1.0f);
    cv::Mat out;
    
    // Destino deseado para el parche blanco (255, 255, 255)
    cv::Scalar target(255.0, 255.0, 255.0);

    if (p == 0.0f)
    {
        // --- MÉTODO WHITE PATCH CLÁSICO ---
        // 1. Convertimos a escala de grises para obtener la luminancia
        cv::Mat gray;
        fsiv_convert_bgr_to_gray(in, gray);
        
        // 2. Buscamos la posición del píxel con máxima luminosidad
        cv::Point max_loc;
        cv::minMaxLoc(gray, nullptr, nullptr, nullptr, &max_loc);
        
        // 3. Extraemos el color BGR exacto de ese punto más luminoso
        cv::Vec3b brightest_pixel = in.at<cv::Vec3b>(max_loc);
        cv::Scalar from(brightest_pixel[0], brightest_pixel[1], brightest_pixel[2]);
        
        // 4. Escalamos la imagen
        out = fsiv_color_rescaling(in, from, target);
    }
    else
    {
        // --- MÉTODO WHITE PATCH ROBUSTO (POR PERCENTIL) ---
        // 1. Convertimos a escala de grises y calculamos su histograma
        cv::Mat gray;
        fsiv_convert_bgr_to_gray(in, gray);
        cv::Mat hist = fsiv_compute_image_histogram(gray);
        
        // 2. Encontramos el umbral correspondiente al percentil (1.0 - p)
        int threshold = fsiv_compute_histogram_percentile(hist, 1.0f - p);
        
        // 3. Creamos una máscara con los píxeles cuya luminancia sea mayor o igual al umbral
        cv::Mat mask = (gray >= threshold);
        
        // 4. Calculamos el color BGR promedio de únicamente los píxeles seleccionados por la máscara
        cv::Scalar mean_from = cv::mean(in, mask);
        
        // 5. Escalamos la imagen basándonos en dicho promedio
        out = fsiv_color_rescaling(in, mean_from, target);
    }
    
    return out;
}