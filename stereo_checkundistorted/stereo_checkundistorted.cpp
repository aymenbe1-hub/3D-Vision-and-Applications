#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>

using namespace cv;
using namespace std;

namespace {

Mat originalStereo;
Mat originalBase;
Mat rectifiedBase;
Mat rectifiedStereo;

bool readCalibration(
    const string& path,
    Mat& leftK,
    Mat& leftD,
    Mat& rightK,
    Mat& rightD,
    Mat& rotation,
    Mat& translation
) {
    FileStorage storage(path, FileStorage::READ);
    if (!storage.isOpened()) {
        cerr << "No se puede abrir la calibracion: " << path << "\n";
        return false;
    }

    storage["LEFT_K"] >> leftK;
    storage["LEFT_D"] >> leftD;
    storage["RIGHT_K"] >> rightK;
    storage["RIGHT_D"] >> rightD;
    storage["R"] >> rotation;
    storage["T"] >> translation;
    storage.release();

    if (leftK.empty() || leftD.empty() || rightK.empty() ||
        rightD.empty() || rotation.empty() || translation.empty()) {
        cerr << "El archivo no contiene todos los parametros necesarios.\n";
        return false;
    }
    return true;
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

void showHorizontalLine(int y) {
    originalStereo = originalBase.clone();
    rectifiedStereo = rectifiedBase.clone();

    if (y >= 0 && y < rectifiedStereo.rows) {
        const Scalar color(0, 0, 255);
        line(
            originalStereo,
            Point(0, y),
            Point(originalStereo.cols - 1, y),
            color,
            1,
            LINE_AA
        );
        line(
            rectifiedStereo,
            Point(0, y),
            Point(rectifiedStereo.cols - 1, y),
            color,
            1,
            LINE_AA
        );
    }

    imshow("Estereo original", originalStereo);
    imshow("Estereo rectificada", rectifiedStereo);
}

void onMouse(int event, int, int y, int, void*) {
    if (event == EVENT_MOUSEMOVE || event == EVENT_LBUTTONDOWN) {
        showHorizontalLine(y);
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        cerr << "Uso: " << argv[0]
             << " <imagen_estereo> <calibracion.yml> [salida_rectificada.jpg]\n";
        return EXIT_FAILURE;
    }

    const string imagePath = argv[1];
    const string calibrationPath = argv[2];

    const Mat stereo = imread(imagePath, IMREAD_COLOR);
    Mat left;
    Mat right;
    if (!splitStereoImage(stereo, left, right)) {
        cerr << "La imagen no es una imagen estereo lado a lado valida.\n";
        return EXIT_FAILURE;
    }

    Mat leftK, leftD, rightK, rightD, rotation, translation;
    if (!readCalibration(
            calibrationPath,
            leftK,
            leftD,
            rightK,
            rightD,
            rotation,
            translation)) {
        return EXIT_FAILURE;
    }

    const Size imageSize = left.size();
    Mat rectLeft, rectRight, projectionLeft, projectionRight, Q;
    Rect validRoiLeft, validRoiRight;

    stereoRectify(
        leftK,
        leftD,
        rightK,
        rightD,
        imageSize,
        rotation,
        translation,
        rectLeft,
        rectRight,
        projectionLeft,
        projectionRight,
        Q,
        CALIB_ZERO_DISPARITY,
        0,
        imageSize,
        &validRoiLeft,
        &validRoiRight
    );

    Mat mapLeftX, mapLeftY, mapRightX, mapRightY;
    initUndistortRectifyMap(
        leftK, leftD, rectLeft, projectionLeft, imageSize,
        CV_32FC1, mapLeftX, mapLeftY
    );
    initUndistortRectifyMap(
        rightK, rightD, rectRight, projectionRight, imageSize,
        CV_32FC1, mapRightX, mapRightY
    );

    Mat leftRectified, rightRectified;
    remap(left, leftRectified, mapLeftX, mapLeftY, INTER_LINEAR);
    remap(right, rightRectified, mapRightX, mapRightY, INTER_LINEAR);

    originalBase = Mat::zeros(imageSize.height, imageSize.width * 2, CV_8UC3);
    left.copyTo(originalBase(Rect(0, 0, imageSize.width, imageSize.height)));
    right.copyTo(
        originalBase(Rect(imageSize.width, 0, imageSize.width, imageSize.height))
    );
    originalStereo = originalBase.clone();

    rectifiedStereo =
        Mat::zeros(imageSize.height, imageSize.width * 2, CV_8UC3);
    leftRectified.copyTo(
        rectifiedStereo(Rect(0, 0, imageSize.width, imageSize.height))
    );
    rightRectified.copyTo(
        rectifiedStereo(Rect(imageSize.width, 0, imageSize.width, imageSize.height))
    );
    rectifiedBase = rectifiedStereo.clone();

    if (argc == 4 && !imwrite(argv[3], rectifiedStereo)) {
        cerr << "No se pudo guardar la imagen rectificada.\n";
        return EXIT_FAILURE;
    }

    namedWindow("Estereo original", WINDOW_NORMAL);
    namedWindow("Estereo rectificada", WINDOW_NORMAL);
    imshow("Estereo original", originalStereo);
    imshow("Estereo rectificada", rectifiedStereo);
    setMouseCallback("Estereo original", onMouse);
    setMouseCallback("Estereo rectificada", onMouse);

    cout << "Mueve el raton sobre cualquiera de las dos ventanas para "
            "dibujar una linea horizontal.\n";
    cout << "Pulsa ESC para salir.\n";
    while (waitKey(30) != 27) {
    }

    destroyAllWindows();
    return EXIT_SUCCESS;
}