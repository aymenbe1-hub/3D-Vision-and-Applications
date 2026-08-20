# Qué he hecho y qué no

fsiv_compute_derivate — implementada correctamente usando cv::Sobel con el tamaño de apertura indicado.
fsiv_compute_gradient_magnitude — implementada usando cv::magnitude sobre los gradientes X e Y.
fsiv_compute_gradient_histogram — implementada usando cv::calcHist sobre la magnitud del gradiente.
fsiv_compute_histogram_percentile — implementada recorriendo el histograma acumulado hasta alcanzar el percentil indicado.
fsiv_compute_histogram_idx_to_value — implementada con la fórmula de interpolación lineal.
fsiv_percentile_edge_detector — implementada umbalizando la magnitud del gradiente según el percentil.
fsiv_otsu_edge_detector — implementada usando cv::threshold con el flag THRESH_OTSU.
fsiv_canny_edge_detector — implementada usando cv::Canny con los gradientes precalculados.
fsiv_thinning_edge_map — implementada con distanceTransform y dilate según el algoritmo de la práctica.
fsiv_compute_ground_truth_image — implementada promediando las anotaciones y aplicando el umbral de consenso.
fsiv_compute_edge_detector_confusion_matrix — implementada con el convenio TP/FN/FP/TN correcto.
fsiv_compute_sensitivity, fsiv_compute_precision, fsiv_compute_F1_score — implementadas correctamente._

# Enlace al vídeo describiendo la práctica


https://youtu.be/nAi490a6DjA
00:00 - Introduction
00:35 - General explanation
01:15- Code explanation
02:16 - Code execution
04:17- Demonstration
08:06 - Conclusion
