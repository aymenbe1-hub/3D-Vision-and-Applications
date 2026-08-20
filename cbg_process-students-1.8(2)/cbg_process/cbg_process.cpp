/*!
  Esto es un esqueleto de programa para usar en las prácticas
  de Visión Artificial.

  Se supone que se utilizará OpenCV.

  Para compilar, puedes ejecutar:
    g++ -Wall -o esqueleto esqueleto.cc `pkg-config opencv --cflags --libs`

*/

#include <iostream>
#include <exception>

// Includes para OpenCV, Descomentar según los módulo utilizados.
#include <opencv2/core/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// #include <opencv2/calib3d/calib3d.hpp>

#include "common_code.hpp"

const cv::String keys =
    "{help h usage ? |      | print this message.}"
    "{i interactive  |      | Activate interactive mode.}"
    "{l luma         |      | process only \"luma\" if color image.}"
    "{c contrast     |1.0   | contrast parameter.}"
    "{b bright       |0.0   | bright parameter.}"
    "{g gamma        |1.0   | gamma parameter.}"
    "{@input         |<none>| input image.}"
    "{@output        |<none>| output image.}";

struct UserData
{
    cv::Mat input;
    cv::Mat output_hist;
    cv::Mat input_hist_image;
    bool interactive;
    cv::Mat output;
    double contrast;
    double bright;
    double gamma;
    bool luma_is_set;
};

cv::Mat draw_histogram(const cv::Mat &hist, int width, int height,
                       const cv::Scalar &color = cv::Scalar(255, 255, 255))
{
    // Crear imagen en negro para dibujar el histograma
    cv::Mat hist_image = cv::Mat::zeros(height, width, CV_8UC3);

    // Verificar que el histograma sea válido
    if (hist.empty() || hist.rows <= 0)
        return hist_image;

    int hist_size = hist.rows;

    // Normalizar el histograma para que se ajuste a la altura de la imagen
    cv::Mat normalized_hist;
    cv::normalize(hist, normalized_hist, 0, height * 0.9, cv::NORM_MINMAX, -1, cv::Mat());

    // Calcular el ancho de cada barra del histograma
    int bin_width = width / hist_size;

    // Dibujar cada barra del histograma
    for (int i = 0; i < hist_size; i++)
    {
        int bin_height = static_cast<int>(normalized_hist.at<float>(i));

        // Coordenadas del rectángulo
        cv::Point pt1(i * bin_width, height);
        cv::Point pt2((i + 1) * bin_width, height - bin_height);

        // Dibujar la barra
        cv::rectangle(hist_image, pt1, pt2, color, -1);
    }

    return hist_image;
}

void process_image(UserData *p)
{
    p->output = fsiv_cbg_process(p->input, p->contrast, p->bright,
                                 p->gamma, p->luma_is_set);
    if (p->interactive)
    {
        cv::Mat output_luma = p->output;
        if (p->output.channels() == 3)
        {
            cv::cvtColor(p->output, output_luma, cv::COLOR_BGR2GRAY);
            std::vector<cv::Mat> channels;
            cv::split(output_luma, channels);
            output_luma = channels[0];
        }
        cv::Mat output_hist;
        const int histSize[] = {256};
        float hrange[] = {0, 256};
        const float *ranges[] = {hrange};
        const int channels[] = {0};
        cv::calcHist(&output_luma, 1, channels, cv::Mat(), output_hist, 1, histSize, ranges);
        cv::Mat output_hist_image = draw_histogram(output_hist, 512, 400, cv::Scalar(0, 0, 255));
        cv::imshow("PROCESSED LUMA HISTOGRAM", output_hist_image);
    }
}

void contrast_trackbar(int pos, void *userdata)
{
    UserData *d = static_cast<UserData *>(userdata);
    d->contrast = float(pos) / 200.0 * 2.0;
    std::cout << "Set contrast to " << d->contrast << std::endl;
    process_image(d);
    cv::imshow("PROCESSED", d->output);
}

void bright_trackbar(int pos, void *userdata)
{
    UserData *d = static_cast<UserData *>(userdata);
    d->bright = (float(pos) - 100.0) / 100.0;
    std::cout << "Set bright to " << d->bright << std::endl;
    process_image(d);
    cv::imshow("PROCESSED", d->output);
}

void gamma_trackbar(int pos, void *userdata)
{
    UserData *d = static_cast<UserData *>(userdata);
    d->gamma = float(pos) / 200.0 * 2.0;
    std::cout << "Set gamma to " << d->gamma << std::endl;
    process_image(d);
    cv::imshow("PROCESSED", d->output);
}

void luma_trackbar(int pos, void *userdata)
{
    UserData *d = static_cast<UserData *>(userdata);
    d->luma_is_set = (pos == 1);
    std::cout << "Set luma mode to state " << d->luma_is_set << std::endl;
    process_image(d);
    cv::imshow("PROCESSED", d->output);
}

int main(int argc, char *const *argv)
{
    int retCode = EXIT_SUCCESS;

    try
    {

        cv::CommandLineParser parser(argc, argv, keys);
        parser.about("Adjust the contrast/bright/gamma parameters of an image. (version 1.8.0)");
        if (parser.has("help"))
        {
            parser.printMessage();
            return 0;
        }

        cv::String input_name = parser.get<cv::String>(0);
        cv::String output_name = parser.get<cv::String>(1);

        if (!parser.check())
        {
            parser.printErrors();
            return 0;
        }

        UserData data;
        data.interactive = parser.has("i");
        data.contrast = parser.get<double>("c");
        data.bright = parser.get<double>("b");
        data.gamma = parser.get<double>("g");
        data.luma_is_set = parser.has("l");
        int c_int = data.contrast / 2.0 * 200;
        int b_int = (data.bright + 1.0) / 2.0 * 200;
        int g_int = data.gamma / 2.0 * 200;
        int l_int = data.luma_is_set ? 1 : 0;

        if (data.contrast < 0.0 || data.contrast > 2.0)
        {
            std::cerr << "Error: contrast parameter has values in [0, 2] interval." << std::endl;
            return EXIT_FAILURE;
        }
        if (data.bright < -1.0 || data.bright > 1.0)
        {
            std::cerr << "Error: bright parameter has values in [-1, 1] interval." << std::endl;
            return EXIT_FAILURE;
        }
        if (data.gamma < 0.0 || data.gamma > 2.0)
        {
            std::cerr << "Error: gamma parameter has values in [0, 2] interval." << std::endl;
            return EXIT_FAILURE;
        }

        data.input = cv::imread(input_name, cv::IMREAD_ANYCOLOR);

        if (data.input.empty())
        {
            std::cerr << "Error: could not open the input image '" << input_name << "'." << std::endl;
            return EXIT_FAILURE;
        }

        data.input.copyTo(data.output);

        int key = 0;
        process_image(&data);

        if (data.interactive)
        {
            cv::namedWindow("ORIGINAL", cv::WINDOW_GUI_EXPANDED + cv::WINDOW_NORMAL);
            cv::resizeWindow("ORIGINAL", cv::Size(800, 600));
            cv::namedWindow("ORIGINAL LUMA HISTOGRAM", cv::WINDOW_GUI_EXPANDED + cv::WINDOW_NORMAL);
            cv::resizeWindow("ORIGINAL LUMA HISTOGRAM", cv::Size(512, 400));

            cv::namedWindow("PROCESSED", cv::WINDOW_GUI_EXPANDED + cv::WINDOW_NORMAL);
            cv::resizeWindow("PROCESSED", cv::Size(800, 600));
            cv::namedWindow("PROCESSED LUMA HISTOGRAM", cv::WINDOW_GUI_EXPANDED + cv::WINDOW_NORMAL);
            cv::resizeWindow("PROCESSED LUMA HISTOGRAM", cv::Size(512, 400));

            cv::namedWindow("PARAMETERS", cv::WINDOW_GUI_EXPANDED + cv::WINDOW_AUTOSIZE);
            cv::createTrackbar("C", "PARAMETERS", &c_int, 200, contrast_trackbar, &data);
            cv::createTrackbar("B", "PARAMETERS", &b_int, 200, bright_trackbar, &data);
            cv::createTrackbar("G", "PARAMETERS", &g_int, 200, gamma_trackbar, &data);
            cv::createTrackbar("Luma", "PARAMETERS", &l_int, 1, luma_trackbar, &data);
            cv::Mat input_luma = data.input;
            if (data.input.channels() == 3)
            {
                cv::cvtColor(data.input, input_luma, cv::COLOR_BGR2GRAY);
                std::vector<cv::Mat> channels;
                cv::split(input_luma, channels);
                input_luma = channels[0];
            }
            cv::Mat input_hist;
            const int histSize[] = {256};
            float hrange[] = {0, 256};
            const float *ranges[] = {hrange};
            const int channels[] = {0};
            cv::calcHist(&input_luma, 1, channels, cv::Mat(), input_hist, 1, histSize, ranges);
            cv::Mat input_hist_image = draw_histogram(input_hist, 512, 400, cv::Scalar(0, 255, 0));

            cv::imshow("ORIGINAL LUMA HISTOGRAM", input_hist_image);
            cv::imshow("ORIGINAL", data.input);
            cv::imshow("PROCESSED", data.output);
            std::cout << "Press '<ENTER>' key to save the result, or '<ESC>' key to exit without saving." << std::endl;

            do
            {
                key = cv::waitKey(0) & 0xff;
            } while (key != 27 && key != 13);
        }

        if (key != 27)
        {
            if (!cv::imwrite(output_name, data.output))
            {
                std::cerr << "Error: could not save the result in file '" << output_name << "'." << std::endl;
                return EXIT_FAILURE;
            }
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
        retCode = EXIT_FAILURE;
    }
    return retCode;
}
