
# Explicación de la técnica y justificación 
1. Parámetros de la Ecuación (I' = c × I^g + b)

Contraste (c): Modifica la imagen de forma lineal. Al aumentar c > 1.0, el histograma se estira, haciendo los claros más claros y los oscuros más oscuros.

Brillo (b): Aplica un offset constante. Desplaza todo el histograma rígidamente hacia la derecha (aclarar) o hacia la izquierda (oscurecer).

Gamma (g): Controla la no linealidad (exponencial). Modifica los tonos medios sin alterar los extremos (blancos y negros puros quedan fijos).
  - g < 1.0: Curva hacia arriba; aclara y rescata detalles ocultos en las sombras (como la vegetación o las ruedas).
  - g > 1.0: Curva hacia abajo; comprime los tonos y oscurece la imagen.

2. Comparación: "Todos los canales" vs. "Solo Luma"

Al observar interactivamente el maillot del ciclista:

Todos los Canales (Modo Full RGB / Luma = 0): La ecuación se aplica a los canales R, G y B de forma independiente. Al alterar sus proporciones relativas, se destruye el matiz (hue) original. El maillot rojo se distorsiona hacia un tono anaranjado/amarillento artificial.

Solo Luma (Modo HSV / Luma = 1): La imagen se convierte a HSV y la ecuación se aplica únicamente al canal V (Luminancia). Como los canales H (matiz) y S (saturación) no se tocan, la iluminación cambia pero el maillot conserva fielmente su color rojo original.

# Lo que he hecho

Funciones de conversión y color: He implementado completamente el paso de la imagen de byte a flotante ([0.0, 1.0]) y viceversa, así como las conversiones de espacio de color entre BGR y HSV en common_code.cpp. Algoritmo principal y modo interactivo: Completada la función fsiv_cbg_process aplicando la ecuación I" = c * I^{g} + b. El código supera el 100% de los tests de test_common_code y funciona de manera interactiva con los Trackbars en cbg_process.cpp.

# Lo que no he hecho
ninguna
# Enlace al vídeo descriptivo

_No olvides poner en la descripción del vídeo los capítulos de la forma:_
'
Capítulos:
00:00 - Introduction
00:15 - General explanation
00:47 - Code explanation
02:27 - Code execution
02:41 - Demonstration
05:10 - Conclusion
