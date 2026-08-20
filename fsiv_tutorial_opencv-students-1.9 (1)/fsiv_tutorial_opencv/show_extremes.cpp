#include <iostream>
#include <exception>

//OpenCV includes
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp> // Necesario para cv::VideoCapture

#include "common_code.hpp"

const char * keys =
    "{help h usage ? |      | print this message}"
    "{w              |20    | Wait time (miliseconds) between frames.}"
    "{v              |      | the input is a video file.}"
    "{c              |      | the input is a camera index.}"    
    "{@input         |<none>| input <fname|int>}"
    ;

// Función auxiliar para calcular e imprimir los extremos de un frame/imagen
void procesar_y_mostrar_extremos(cv::Mat const& img)
{
    std::vector<double> min_v, max_v;
    std::vector<cv::Point> min_loc, max_loc;

    // Llamamos a la función implementada en common_code
    fsiv_find_min_max_loc_2(img, min_v, max_v, min_loc, max_loc);

    std::cout << "-------------------------------------------" << std::endl;
    for (size_t c = 0; c < min_v.size(); ++c)
    {
        std::cout << "Canal " << c << ":"
                  << " Min=" << min_v[c] << " en " << min_loc[c]
                  << " | Max=" << max_v[c] << " en " << max_loc[c] 
                  << std::endl;
    }
}

int
main (int argc, char* const* argv)
{
  int retCode=EXIT_SUCCESS;
  
  try {    

      cv::CommandLineParser parser(argc, argv, keys);
      parser.about("Show the extremes values and their locations.");
      if (parser.has("help"))
      {
          parser.printMessage();
          return 0;
      }
      bool is_video = parser.has("v");
      bool is_camera = parser.has("c");
      int wait = parser.get<int>("w");
      cv::String input = parser.get<cv::String>("@input");
      if (!parser.check())
      {
          parser.printErrors();
          return 0;
      }

      // Nombre de la ventana donde mostraremos el contenido gráfico
      const std::string window_name = "Imagen / Video Entrada";
      cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);

      if (is_video || is_camera)
      {
          // Modo vídeo o cámara
          cv::VideoCapture cap;
          
          if (is_camera)
          {
              // Si es cámara, pasamos el string convertido a entero (índice de cámara)
              int camera_idx = std::stoi(input);
              cap.open(camera_idx);
          }
          else
          {
              // Si es archivo de vídeo, abrimos la ruta directamente
              cap.open(input);
          }

          if (!cap.isOpened())
          {
              std::cerr << "Error: No se pudo abrir la fuente de vídeo/cámara: " << input << std::endl;
              return EXIT_FAILURE;
          }

          cv::Mat frame;
          while (true)
          {
              cap >> frame; // Captura el siguiente frame
              if (frame.empty())
                  break; // Fin del vídeo o fallo de lectura

              // Mostramos el frame en la ventana gráfica
              cv::imshow(window_name, frame);

              // Procesamos e imprimimos en la consola los extremos por canal
              procesar_y_mostrar_extremos(frame);

              // Espera los milisegundos indicados por parámetro (-w). Si se pulsa ESC (27), sale.
              char key = (char)cv::waitKey(wait);
              if (key == 27) 
                  break;
          }
          cap.release();
      }
      else
      {
          // Modo imagen estática
          cv::Mat img = cv::imread(input, cv::IMREAD_ANYCOLOR);
          if (img.empty())
          {
              std::cerr << "Error: No se pudo cargar la imagen: " << input << std::endl;
              return EXIT_FAILURE;
          }

          // Visualizamos la imagen
          cv::imshow(window_name, img);

          // Procesamos e imprimimos extremos
          procesar_y_mostrar_extremos(img);

          // En imágenes estáticas, esperamos indefinidamente hasta que el usuario pulse una tecla
          cv::waitKey(0);
      }

      cv::destroyAllWindows();
  }
  catch (std::exception& e)
  {
    std::cerr << "Caught exception: " << e.what() << std::endl;
    retCode = EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Caught unknown exception!" << std::endl;
    retCode = EXIT_FAILURE;
  }
  return retCode;
}