#include <opencv2/opencv.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace cv;
using namespace std;

namespace {

constexpr int BOARD_COLS = 7;       // esquinas interiores
constexpr int BOARD_ROWS = 5;       // esquinas interiores
constexpr double SQUARE_SIZE = 0.02875; // metros

vector<Point3f> makeBoardPoints() {
    vector<Point3f> points;
    points.reserve(BOARD_COLS * BOARD_ROWS);

    for (int row = 0; row < BOARD_ROWS; ++row) {
        for (int col = 0; col < BOARD_COLS; ++col) {
            points.emplace_back(
                static_cast<float>(col * SQUARE_SIZE),
                static_cast<float>(row * SQUARE_SIZE),
                0.0f
            );
        }
    }
    return points;
}

bool splitStereoImage(const Mat& stereo, Mat& left, Mat& right) {
    if (stereo.empty() || stereo.cols < 2 || stereo.cols % 2 != 0) {
        return false;
    }

    const int width = stereo.cols / 2;
    left = stereo(Rect(0, 0, width, stereo.rows)).clone();
    right = stereo(Rect(width, 0, width, stereo.rows)).clone();
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "Uso: " << argv[0]
             << " <directorio_imagenes_estereo> <calibracion.yml>\n";
        cerr << "Ejemplo: " << argv[0]
             << " datos/calibracion stereo_calib.yml\n";
        return EXIT_FAILURE;
    }

    const string imageDirectory = argv[1];
    const string outputFile = argv[2];

    vector<String> files;
    glob(imageDirectory + "/*.jpg", files, false); 

    if (files.empty()) {
        cerr << "No se encontraron imagenes JPG, JPEG o PNG en "
             << imageDirectory << ".\n";
        return EXIT_FAILURE;
    }

    const Size boardSize(BOARD_COLS, BOARD_ROWS);
    const vector<Point3f> boardPoints = makeBoardPoints();
    vector<vector<Point3f>> objectPoints;
    vector<vector<Point2f>> imagePointsLeft;
    vector<vector<Point2f>> imagePointsRight;
    Size imageSize;

    const TermCriteria criteria(
        TermCriteria::MAX_ITER + TermCriteria::EPS, 60, 1e-6
    );

    for (const String& file : files) {
        const Mat stereo = imread(file, IMREAD_GRAYSCALE);
        Mat left;
        Mat right;

        if (!splitStereoImage(stereo, left, right)) {
            cerr << "Imagen estereo no valida: " << file << "\n";
            continue;
        }

        if (imageSize.empty()) {
            imageSize = left.size();
        }
        if (left.size() != imageSize || right.size() != imageSize) {
            cerr << "Tamano diferente, se ignora: " << file << "\n";
            continue;
        }

        vector<Point2f> cornersLeft;
        vector<Point2f> cornersRight;
        const int flags =
            CALIB_CB_ADAPTIVE_THRESH |
            CALIB_CB_NORMALIZE_IMAGE |
            CALIB_CB_FAST_CHECK;

        const bool foundLeft =
            findChessboardCorners(left, boardSize, cornersLeft, flags);
        const bool foundRight =
            findChessboardCorners(right, boardSize, cornersRight, flags);

        if (!foundLeft || !foundRight) {
            cerr << "Tablero no encontrado en ambos lados: " << file << "\n";
            continue;
        }

        cornerSubPix(
            left, cornersLeft, Size(11, 11), Size(-1, -1), criteria
        );
        cornerSubPix(
            right, cornersRight, Size(11, 11), Size(-1, -1), criteria
        );

        imagePointsLeft.push_back(cornersLeft);
        imagePointsRight.push_back(cornersRight);
        objectPoints.push_back(boardPoints);

        cout << "Imagen valida: " << file << "\n";
    }

    if (objectPoints.size() < 3) {
        cerr << "Se necesitan al menos 3 imagenes validas para calibrar.\n";
        return EXIT_FAILURE;
    }

    Mat leftK, leftD, rightK, rightD;
    Mat rotation, translation, essential, fundamental;

    const double rms = stereoCalibrate(
        objectPoints,
        imagePointsLeft,
        imagePointsRight,
        leftK,
        leftD,
        rightK,
        rightD,
        imageSize,
        rotation,
        translation,
        essential,
        fundamental,
        CALIB_RATIONAL_MODEL,
        criteria
    );

    FileStorage storage(outputFile, FileStorage::WRITE);
    if (!storage.isOpened()) {
        cerr << "No se puede escribir el archivo: " << outputFile << "\n";
        return EXIT_FAILURE;
    }

    // Estos nombres son la interfaz común utilizada por las otras prácticas.
    storage << "LEFT_K" << leftK;
    storage << "LEFT_D" << leftD;
    storage << "RIGHT_K" << rightK;
    storage << "RIGHT_D" << rightD;
    // Alias compatibles con versiones del enunciado que usan espacios.
    storage << "LEFT K" << leftK;
    storage << "LEFT D" << leftD;
    storage << "RIGHT K" << rightK;
    storage << "RIGHT D" << rightD;
    storage << "R" << rotation;
    storage << "T" << translation;
    storage << "E" << essential;
    storage << "F" << fundamental;
    storage << "image_width" << imageSize.width;
    storage << "image_height" << imageSize.height;
    storage << "board_cols" << BOARD_COLS;
    storage << "board_rows" << BOARD_ROWS;
    storage << "square_size" << SQUARE_SIZE;
    storage << "rms" << rms;
    storage.release();

    cout << "\nCalibracion terminada.\n";
    cout << "Imagenes validas: " << objectPoints.size() << "\n";
    cout << "Error RMS: " << rms << "\n";
    cout << "Parametros guardados en: " << outputFile << "\n";
    return EXIT_SUCCESS;
}