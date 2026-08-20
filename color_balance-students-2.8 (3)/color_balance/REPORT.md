# Justificación técnica
1. Con P = 0 (White Patch Clásico)

Qué observo: La imagen apenas cambia; el fuerte tinte anaranjado/amarillento de la escena se mantiene casi idéntico al de la ventana INPUT.

Justificación técnica: El algoritmo busca el píxel con la máxima luminancia absoluta de la imagen. En esta escena, los píxeles más brillantes se localizan en el centro de la llama de las velas, los cuales ya están completamente saturados en la captura con valores espurios muy cercanos a un blanco puro (255, 255, 255). Como el valor del píxel detectado ya es blanco, el factor de escala α calculado es prácticamente 1.0 para todos los canales, provocando que el algoritmo falle y no aplique corrección alguna.

2. Con P entre 1 y 25 (White Patch Robusto por Percentil)

Qué observo: A medida que desplazas la barra (por ejemplo, al 25%), el tono anaranjado desaparece de golpe. La camisa del hombre, el mantel y la etiqueta de la botella de vino recuperan un color blanco neutro e impecable.

Justificación técnica: Al promediar los colores de un porcentaje de los píxeles más brillantes, el método se vuelve inmune a los outliers (como las llamas de las velas). Al tomar una región representativa de alta luminosidad (que incluye la camisa y las etiquetas), se estima con precisión el verdadero sesgo de la iluminación ambiente. El factor de escala contrarresta el exceso de ganancia en los canales cálidos, logrando un equilibrado cromático excelente y fiel a la realidad.

3. Con P = 100 (Mundo Gris / Gray World)

Qué observo: Toda la escena cambia por completo hacia un balance mucho más frío y equilibrado de forma automática.

Justificación técnica: Este enfoque no busca zonas blancas, sino que asume la hipótesis de que el promedio de todos los canales de una escena natural tiende a un gris neutro (128, 128, 128). Al calcular la media global de la imagen (que está muy desviada hacia el rojo/amarillo por las velas) y forzarla a desplazarse hacia el gris neutro, el algoritmo neutraliza por completo el dominante cálido. Es una técnica sumamente robusta porque utiliza la información de todos y cada uno de los píxeles de la imagen para autocorregirse.
# Lo que he hecho
-Reescalado de color y Mundo Gris: He implementado completamente la función genérica de reescalado canal a canal fsiv_color_rescaling y el algoritmo de balance de color automático fsiv_gray_world_color_balance en common_code.cpp.
-White Patch Robusto por Percentil: Se han desarrollado con éxito las funciones auxiliares para pasar a escala de grises, calcular el histograma y determinar el valor del percentil acumulado (fsiv_compute_histogram_percentile). Con ello, se ha programado el método White Patch clásico (P=0) y la versión robusta mediante el uso de máscaras para promediar el porcentaje de los píxeles más luminosos.
-Validación e Interfaz: El código pasa el 100% de los tests automatizados (test_common_code) y funciona correctamente de forma interactiva en el programa principal utilizando la opción -p=0 (u Omisión) para evitar el conflicto del parser de comandos de OpenCV.
# Lo que no he hecho
Ninguna
# Enlace al vídeo descriptivo

https://youtu.be/3eS9H_58jr0
'
00:00 Introducción 00:15
00:14 Explicación general 
01:15 Explicación del codigo 
02:25 Ejecución del codigo
02:49 demostración
04:44 Conclusión
