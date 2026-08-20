# Camera Calibration \& Augmented Reality

1-Lo que he hecho

Calibración de la cámara: Implementado calibrate.cpp, que detecta las esquinas del tablero en las imágenes de ./calibration, calcula los parámetros intrínsecos y los guarda en calibration.yml junto con el error RMS de reproyección.



Realidad aumentada: Implementado augReal.cpp, que procesa el vídeo indicado detectando el tablero en cada frame (findChessboardCorners + cornerSubPix), estima la pose con solvePnP y dibuja los ejes 3D (X rojo, Y verde, Z azul) mediante projectPoints. Incluye controles interactivos (q/ESC salir, p pausar, s guardar frame).



2-compilacion 

mkdir build \\\&\\\& cd build
cmake ..
make
./calibrate 5 8 ../calibration ../calibration.yml

./augReal 2 ../calibration.yml ../augreal.mp4




