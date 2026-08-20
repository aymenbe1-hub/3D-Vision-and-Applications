# Justificación Técnica
El objetivo de esta práctica es aumentar la nitidez de una imagen digital potenciando sus altas frecuencias (detalles finos y bordes). La operación matemática general responde a la ecuación:

O = I + I_h

Donde I es la imagen original e I_h representa la versión de alta frecuencia. Dependiendo de los parámetros seleccionados en la interfaz, el comportamiento técnico varía de la siguiente forma:

1. Filtro Laplaciano de 4 puntos (FILTER = 0) frente a 8 puntos (FILTER = 1)

Análisis Técnico: El operador Laplaciano (∇²) calcula la segunda derivada del espacio bidimensional para detectar cambios bruscos en los niveles de gris.

El filtro de 4 puntos (LAP_4) utiliza una máscara direccional ortogonal (arriba, abajo, izquierda y derecha). Solo extrae variaciones en los ejes principales de la cuadrícula de píxeles.

El filtro de 8 puntos (LAP_8) añade las cuatro diagonales del entorno del píxel.

Resultado e Impacto: Al incluir el análisis en los 360 grados de la vecindad del píxel, LAP_8 recoge una cantidad drásticamente mayor de altas frecuencias que LAP_4. Como consecuencia, al sumarlo de nuevo a la imagen, el realce de los bordes y texturas es mucho más enérgico, agresivo y contrastado con LAP_8.

2. Filtro DoG - Diferencia de Gaussianas (FILTER = 2) y el uso de los Radios (R1 y R2)

Análisis Técnico: El Laplaciano tradicional tiene el inconveniente de que es un filtro paso alto puro, por lo que amplifica de igual manera los detalles reales y el ruido granular indeseado. El filtro DoG (Difference of Gaussians) soluciona esto actuando como un filtro paso banda.

∇² ≈ DoG(σ1, σ2) = G_σ2 - G_σ1

Al restar dos funciones de desenfoque Gaussiano de distintos tamaños (donde R1 representa el radio interno σ1 y R2 el radio externo σ2), eliminamos las frecuencias muy bajas (las zonas homogéneas) y las frecuencias críticamente altas (el ruido fino).

Resultado e Impacto: Al modificar las barras R1 y R2 (manteniendo siempre 0 < R1 < R2), seleccionamos la anchura de la "banda" de frecuencias que deseamos aislar. Al aumentar la distancia entre los radios, el filtro captura estructuras de bordes más gruesas y genera un enfoque más natural y limpio que el Laplaciano estándar.

3. Expansión por Ceros (CIRC = 0) frente a Expansión Circular (CIRC = 1)

Análisis Técnico: Al aplicar convoluciones espaciales mediante máscaras de filtrado de tamaño N × N, el centro del filtro no puede posicionarse sobre los píxeles perimetrales de la imagen sin salirse de los límites físicos de esta. Para solucionarlo, extendemos el tamaño de la imagen un radio r.

Relleno con Ceros (fsiv_fill_expansion): Añade un borde perimetral constante con el valor 0 (negro). Esto crea una discontinuidad artificial drástica entre los píxeles reales exteriores de la foto y el fondo negro impuesto. Al pasar el filtro de enfoque por ahí, el algoritmo interpreta falsamente esa caída brusca como si fuera un borde real de la escena, generando una línea brillante u oscura muy artificial en los bordes exactos de la imagen final.

Expansión Circular (fsiv_circular_expansion): Conecta de forma toroidal los extremos opuestos de la matriz (el borde superior se alimenta del inferior y el derecho del izquierdo). Esto reduce los saltos extremos y ficticios de intensidad.

Resultado e Impacto: Al activar CIRC = 1 los artefactos visuales y distorsiones del marco exterior de la fotografía desaparecen, garantizando que el tratamiento matemático en las fronteras sea continuo y coherente.

# Qué he hecho y qué no
-Construcción de filtros espaciales: He implementado las funciones para generar kernels Gaussianos bidimensionales fsiv_create_gaussian_filter mediante el producto externo de vectores, así como las matrices rígidas para los operadores Laplacianos de 4 y 8 puntos (fsiv_create_lap4_filter y fsiv_create_lap8_filter) ajustando la simetría de sus signos.

-Diseño del filtro DoG: He programado el filtro de Diferencia de Gaussianas fsiv_create_dog_filter, aplicando un padding constante de ceros sobre la Gaussiana de menor radio  para igualar sus dimensiones con la de mayor radio ($R_2$) antes de realizar la resta matricial.

-Estrategias de expansión y convolución: Se han desarrollado con éxito las funciones de expansión de bordes por relleno de ceros constantes (fsiv_fill_expansion) y por duplicación cíclica/circular (fsiv_circular_expansion). Con ellas, implementé el algoritmo central fsiv_image_sharpening combinando la señal identidad con el Laplaciano/DoG mediante cv::filter2D con la bandera cv::BORDER_ISOLATED y recortando la región de interés final.

-Validación: El código supera con éxito el 100% de los tests automatizados (test_common_code).

# Enlace al vídeo describiendo la práctica

https://youtu.be/WVUgolqlZts


```
Capítulos:

00:00 - Introduction
00:12 - General explanation
01:17- Code explanation
02:38 - Code execution
02:59 - Demonstration
05:02 - Conclusion
