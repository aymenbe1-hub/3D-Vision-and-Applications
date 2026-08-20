#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "common_code.hpp"

/**
 * Calcula las derivadas parciales Δx, Δy usando filtro de Sobel,
 * con suavizado gaussiano previo si g_r > 0.
 */
void fsiv_compute_derivate(cv::Mat const &img, cv::Mat &dx, cv::Mat &dy,
                           int g_r, int s_ap)
{
    CV_Assert(img.type() == CV_8UC1);

    cv::Mat blurred;
    if (g_r > 0)
    {
        int ksize = 2 * g_r + 1;
        cv::GaussianBlur(img, blurred, cv::Size(ksize, ksize), 0.0);
    }
    else
    {
        blurred = img;
    }

    // s_ap ya es el tamaño de apertura del kernel de Sobel (1, 3, 5, 7…).
    // El llamador (edge_detector.cpp) ya hace la conversión 2*radio+1 antes de llamar.
    // Se usa directamente sin transformación adicional.
    cv::Sobel(blurred, dx, CV_32FC1, 1, 0, s_ap);
    cv::Sobel(blurred, dy, CV_32FC1, 0, 1, s_ap);

    CV_Assert(dx.size() == img.size());
    CV_Assert(dy.size() == dx.size());
    CV_Assert(dx.type() == CV_32FC1);
    CV_Assert(dy.type() == CV_32FC1);
}

/**
 * Calcula la magnitud del gradiente G = sqrt(Δx² + Δy²).
 */
void fsiv_compute_gradient_magnitude(cv::Mat const &dx, cv::Mat const &dy,
                                     cv::Mat &gradient)
{
    CV_Assert(dx.size() == dy.size());
    CV_Assert(dx.type() == CV_32FC1);
    CV_Assert(dy.type() == CV_32FC1);

    cv::magnitude(dx, dy, gradient);

    CV_Assert(gradient.size() == dx.size());
    CV_Assert(gradient.type() == CV_32FC1);
}

/**
 * Calcula el histograma de la magnitud del gradiente en el rango [0, maxG).
 */
void fsiv_compute_gradient_histogram(cv::Mat const &gradient, int n_bins,
                                     cv::Mat &hist, float &max_gradient)
{
    CV_Assert(gradient.type() == CV_32FC1);
    CV_Assert(n_bins > 1);

    double max_val;
    cv::minMaxLoc(gradient, nullptr, &max_val);
    max_gradient = static_cast<float>(max_val);

    if (max_gradient > 0.0f)
    {
        int histSize[]        = { n_bins };
        float range[]         = { 0.0f, max_gradient };
        const float *ranges[] = { range };
        int channels[]        = { 0 };

        cv::calcHist(&gradient, 1, channels, cv::Mat(), hist, 1,
                     histSize, ranges, true, false);
    }
    else
    {
        hist = cv::Mat::zeros(n_bins, 1, CV_32FC1);
    }

    CV_Assert(hist.type() == CV_32FC1);
    CV_Assert(hist.rows == n_bins && hist.cols == 1);
}

/**
 * Devuelve el índice del bin del histograma que corresponde al percentil dado.
 * Recorre el histograma acumulando frecuencias hasta alcanzar
 * total_pixels * percentile.
 */
int fsiv_compute_histogram_percentile(cv::Mat const &hist, float percentile)
{
    CV_Assert(hist.type() == CV_32FC1);
    CV_Assert(hist.cols == 1);
    CV_Assert(0.0f <= percentile && percentile <= 1.0f);

    int idx = 0;
    double total_pixels = cv::sum(hist)[0];

    if (total_pixels > 0.0)
    {
        double target      = total_pixels * static_cast<double>(percentile);
        double accumulated = 0.0;

        for (int i = 0; i < hist.rows; ++i)
        {
            accumulated += static_cast<double>(hist.at<float>(i, 0));
            if (accumulated >= target)
            {
                idx = i;
                break;
            }
        }
    }

    return idx;
}

/**
 * Mapea un índice del histograma [0, n_bins) al rango de valores float
 * [min_gradient, max_gradient).
 */
float fsiv_histogram_idx_to_value(int idx, int n_bins, float max_gradient,
                                   float min_gradient)
{
    return min_gradient +
           (static_cast<float>(idx) * (max_gradient - min_gradient) /
            static_cast<float>(n_bins));
}

/**
 * Detector de bordes basado en percentil.
 *
 * Calcula el histograma de gradiente, encuentra el umbral th (percentil),
 * y clasifica como borde los píxeles con gradiente > th.
 */
void fsiv_percentile_edge_detector(cv::Mat const &gradient, cv::Mat &edges,
                                   float percentile, int n_bins)
{
    CV_Assert(gradient.type() == CV_32FC1);

    cv::Mat hist;
    float max_gradient = 0.0f;
    fsiv_compute_gradient_histogram(gradient, n_bins, hist, max_gradient);

    int idx       = fsiv_compute_histogram_percentile(hist, percentile);
    float th_val  = fsiv_histogram_idx_to_value(idx, n_bins, max_gradient, 0.0f);

    cv::threshold(gradient, edges, static_cast<double>(th_val), 255.0,
                  cv::THRESH_BINARY);
    edges.convertTo(edges, CV_8UC1);

    CV_Assert(edges.type() == CV_8UC1);
    CV_Assert(edges.size() == gradient.size());
}

/**
 * Detector de bordes usando el método de Otsu.
 *
 * Normaliza el gradiente a 8 bits y aplica umbralización automática de Otsu.
 */
void fsiv_otsu_edge_detector(cv::Mat const &gradient, cv::Mat &edges)
{
    CV_Assert(gradient.type() == CV_32FC1);

    double max_val;
    cv::minMaxLoc(gradient, nullptr, &max_val);

    if (max_val > 0.0)
    {
        cv::Mat grad_8u;
        gradient.convertTo(grad_8u, CV_8UC1, 255.0 / max_val);
        cv::threshold(grad_8u, edges, 0, 255,
                      cv::THRESH_BINARY | cv::THRESH_OTSU);
    }
    else
    {
        edges = cv::Mat::zeros(gradient.size(), CV_8UC1);
    }

    CV_Assert(edges.type() == CV_8UC1);
    CV_Assert(edges.size() == gradient.size());
}

/**
 * Detector de Canny basado en percentiles.
 *
 * Recibe las derivadas dx, dy (CV_32FC1). Calcula la magnitud del gradiente
 * para determinar los umbrales absolutos th_low y th_high a partir de los
 * percentiles dados. Llama a cv::Canny con la variante que acepta dx/dy en
 * formato CV_16SC1.
 */
void fsiv_canny_edge_detector(cv::Mat const &dx, cv::Mat const &dy,
                              cv::Mat &edges, float th_low, float th_high,
                              int n_bins)
{
    CV_Assert(dx.type() == CV_32FC1);
    CV_Assert(dy.type() == CV_32FC1);
    CV_Assert(dx.size() == dy.size());
    CV_Assert(th_low < th_high);

    // 1. Calcular magnitud del gradiente para obtener los umbrales reales
    cv::Mat gradient;
    cv::magnitude(dx, dy, gradient);

    cv::Mat hist;
    float max_gradient = 0.0f;
    fsiv_compute_gradient_histogram(gradient, n_bins, hist, max_gradient);

    int   idx_high = fsiv_compute_histogram_percentile(hist, th_high);
    int   idx_low  = fsiv_compute_histogram_percentile(hist, th_low);

    double threshold_high = static_cast<double>(
        fsiv_histogram_idx_to_value(idx_high, n_bins, max_gradient, 0.0f));
    double threshold_low  = static_cast<double>(
        fsiv_histogram_idx_to_value(idx_low,  n_bins, max_gradient, 0.0f));

    // 2. Convertir derivadas a CV_16SC1 para la variante dx/dy de cv::Canny
    cv::Mat dx_16s, dy_16s;
    dx.convertTo(dx_16s, CV_16SC1);
    dy.convertTo(dy_16s, CV_16SC1);

    // 3. Llamar a Canny con la sobrecarga que acepta derivadas precalculadas
    //    L2gradient=true para usar norma L2 consistente con magnitude()
    cv::Canny(dx_16s, dy_16s, edges, threshold_low, threshold_high, true);

    CV_Assert(edges.type() == CV_8UC1);
    CV_Assert(edges.size() == dx.size());
}

/**
 * Adelgazamiento de bordes ("Thinning") mediante transformada de distancia.
 *
 * Algoritmo (según enunciado):
 *   1. TF  = distanceTransform(edges)
 *   2. TF_dil = dilate(TF, kernel 3x3)  → cada píxel = máximo de su entorno 3x3
 *   3. Máscara local_max: píxeles donde TF == TF_dil  (son máximos locales)
 *   4. edges_thin = edges AND local_max
 */
cv::Mat fsiv_thinning_edge_map(cv::Mat const &edges)
{
    CV_Assert(edges.type() == CV_8UC1);

    // Paso 1: Transformada de distancia L2
    cv::Mat TF;
    cv::distanceTransform(edges, TF, cv::DIST_L2, cv::DIST_MASK_3);

    // Paso 2: Dilatar para obtener el máximo local en vecindad 3x3
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat TF_dil;
    cv::dilate(TF, TF_dil, kernel);

    // Paso 3: Máscara de máximos locales: TF == TF_dil
    cv::Mat local_max;
    cv::compare(TF, TF_dil, local_max, cv::CMP_EQ);  // 255 donde es máximo

    // Paso 4: Intersección con el mapa de bordes original
    cv::Mat thinned;
    cv::bitwise_and(edges, local_max, thinned);

    CV_Assert(thinned.type() == CV_8UC1);
    CV_Assert(thinned.size() == edges.size());

    return thinned;
}

/**
 * Genera la imagen Ground Truth (GT) a partir de la imagen de consenso.
 *
 * GT(x,y) = 255 si consensus(x,y) >= (min_consensus/100) * max_consensus_val
 *         = 0   en caso contrario
 */
void fsiv_compute_ground_truth_image(cv::Mat const &consensus_img,
                                     float min_consensus, cv::Mat &gt)
{
    CV_Assert(consensus_img.type() == CV_8UC1);

    double max_val;
    cv::minMaxLoc(consensus_img, nullptr, &max_val);

    double thresh = (static_cast<double>(min_consensus) / 100.0) * max_val;
    cv::threshold(consensus_img, gt, thresh, 255.0, cv::THRESH_BINARY);
    gt.convertTo(gt, CV_8UC1);

    CV_Assert(gt.size() == consensus_img.size());
    CV_Assert(gt.type() == CV_8UC1);
}

/**
 * Calcula la matriz de confusión 2x2.
 *
 * Convenio (filas = GT, columnas = predicción, clase positiva primero):
 *   cm[0,0] = TP  (gt=edge,     pred=edge)
 *   cm[0,1] = FN  (gt=edge,     pred=non-edge)
 *   cm[1,0] = FP  (gt=non-edge, pred=edge)
 *   cm[1,1] = TN  (gt=non-edge, pred=non-edge)
 *
 * Fila 0 = clase borde (positiva): [TP, FN]
 * Fila 1 = clase no-borde (negativa): [FP, TN]
 * Sensibilidad  = cm[0,0] / (cm[0,0] + cm[0,1])  → lectura por fila 0
 * Precisión     = cm[0,0] / (cm[0,0] + cm[1,0])  → lectura por columna 0
 */
void fsiv_compute_edge_detector_confusion_matrix(cv::Mat const &gt,
                                                  cv::Mat const &pred,
                                                  cv::Mat &cm)
{
    CV_Assert(gt.type() == CV_8UC1);
    CV_Assert(pred.type() == CV_8UC1);
    CV_Assert(gt.size() == pred.size());

    cm = cv::Mat::zeros(2, 2, CV_32FC1);

    float tp = 0.0f, fp = 0.0f, fn = 0.0f, tn = 0.0f;

    for (int r = 0; r < gt.rows; ++r)
    {
        const uchar *gt_row   = gt.ptr<uchar>(r);
        const uchar *pred_row = pred.ptr<uchar>(r);

        for (int c = 0; c < gt.cols; ++c)
        {
            bool is_gt   = (gt_row[c]   != 0);
            bool is_pred = (pred_row[c] != 0);

            if       ( is_gt &&  is_pred) tp++;   // borde real, detectado
            else if  (!is_gt &&  is_pred) fp++;   // no borde, detectado como borde
            else if  ( is_gt && !is_pred) fn++;   // borde real, no detectado
            else                          tn++;   // no borde, no detectado
        }
    }

    // Fila 0 = clase borde (positiva):    [TP, FN]
    // Fila 1 = clase no-borde (negativa): [FP, TN]
    cm.at<float>(0, 0) = tp;
    cm.at<float>(0, 1) = fn;
    cm.at<float>(1, 0) = fp;
    cm.at<float>(1, 1) = tn;

    CV_Assert(cm.type() == CV_32FC1);
    CV_Assert(cv::abs(cv::sum(cm)[0] -
                      static_cast<double>(gt.rows * gt.cols)) < 1.0e-3);
}

/**
 * Sensibilidad (Recall) = TP / (TP + FN)
 * Mide qué fracción de bordes reales fue detectada.
 *
 * Convenio: cm[0,0]=TP, cm[0,1]=FN, cm[1,0]=FP, cm[1,1]=TN
 * → Lectura por la fila 0 (clase borde): TP / (TP + FN)
 */
float fsiv_compute_sensitivity(cv::Mat const &cm)
{
    CV_Assert(cm.type() == CV_32FC1);
    CV_Assert(cm.size() == cv::Size(2, 2));

    float tp = cm.at<float>(0, 0);
    float fn = cm.at<float>(0, 1);   // FN está en fila 0, col 1

    if ((tp + fn) <= 0.0f) return 0.0f;
    return tp / (tp + fn);
}

/**
 * Precisión = TP / (TP + FP)
 * Mide qué fracción de los bordes detectados son bordes reales.
 *
 * Convenio: cm[0,0]=TP, cm[0,1]=FN, cm[1,0]=FP, cm[1,1]=TN
 * → Lectura por la columna 0 (predicción=borde): TP / (TP + FP)
 */
float fsiv_compute_precision(cv::Mat const &cm)
{
    CV_Assert(cm.type() == CV_32FC1);
    CV_Assert(cm.size() == cv::Size(2, 2));

    float tp = cm.at<float>(0, 0);
    float fp = cm.at<float>(1, 0);   // FP está en fila 1, col 0

    if ((tp + fp) <= 0.0f) return 0.0f;
    return tp / (tp + fp);
}

/**
 * F1 = 2 * (Precision * Sensibilidad) / (Precision + Sensibilidad)
 * Media armónica entre precisión y sensibilidad.
 */
float fsiv_compute_F1_score(cv::Mat const &cm)
{
    CV_Assert(cm.type() == CV_32FC1);
    CV_Assert(cm.size() == cv::Size(2, 2));

    float sensitivity = fsiv_compute_sensitivity(cm);
    float precision   = fsiv_compute_precision(cm);

    if ((precision + sensitivity) <= 0.0f) return 0.0f;
    return 2.0f * (precision * sensitivity) / (precision + sensitivity);
}
