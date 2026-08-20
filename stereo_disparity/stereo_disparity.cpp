#include <opencv2/opencv.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace cv;
using namespace std;

namespace {

struct StereoCalibration {
    Mat leftK;
    Mat leftD;
    Mat rightK;
    Mat rightD;
    Mat rotation;
    Mat translation;
};

bool readCalibration(const string& path, StereoCalibration& calibration) {
    FileStorage storage(path, FileStorage::READ);
    if (!storage.isOpened()) {
        cerr << "No se puede abrir la calibracion: " << path << "\n";
        return false;
    }

    storage["LEFT_K"] >> calibration.leftK;
    storage["LEFT_D"] >> calibration.leftD;
    storage["RIGHT_K"] >> calibration.rightK;
    storage["RIGHT_D"] >> calibration.rightD;
    storage["R"] >> calibration.rotation;
    storage["T"] >> calibration.translation;
    storage.release();

    if (calibration.leftK.empty() || calibration.leftD.empty() ||
        calibration.rightK.empty() || calibration.rightD.empty() ||
        calibration.rotation.empty() || calibration.translation.empty()) {
        cerr << "Faltan parametros en el archivo de calibracion.\n";
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

bool writeObj(const string& path, const vector<Point3f>& points) {
    ofstream output(path);
    if (!output.is_open()) {
        return false;
    }

    for (const Point3f& point : points) {
        output << "v " << point.x << " " << point.y << " "
               << point.z << "\n";
    }
    return true;
}

bool rectify(
    const StereoCalibration& calibration,
    const Mat& left,
    const Mat& right,
    Mat& leftRectified,
    Mat& rightRectified
) {
    if (left.size() != right.size()) {
        return false;
    }

    const Size imageSize = left.size();
    Mat rectLeft, rectRight, projectionLeft, projectionRight, Q;
    Rect validRoiLeft, validRoiRight;

    stereoRectify(
        calibration.leftK,
        calibration.leftD,
        calibration.rightK,
        calibration.rightD,
        imageSize,
        calibration.rotation,
        calibration.translation,
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
        calibration.leftK, calibration.leftD, rectLeft, projectionLeft,
        imageSize, CV_32FC1, mapLeftX, mapLeftY
    );
    initUndistortRectifyMap(
        calibration.rightK, calibration.rightD, rectRight, projectionRight,
        imageSize, CV_32FC1, mapRightX, mapRightY
    );

    remap(left, leftRectified, mapLeftX, mapLeftY, INTER_LINEAR);
    remap(right, rightRectified, mapRightX, mapRightY, INTER_LINEAR);
    return true;
}

Mat computeDisparity(const Mat& left, const Mat& right) {
    Mat leftGray, rightGray;
    cvtColor(left, leftGray, COLOR_BGR2GRAY);
    cvtColor(right, rightGray, COLOR_BGR2GRAY);

    // El enunciado pide StereoBM. 64 es divisible entre 16 y 9 es impar.
    Ptr<StereoBM> matcher = StereoBM::create(64, 9);

    Mat disparity16;
    matcher->compute(leftGray, rightGray, disparity16);

    Mat disparity32;
    disparity16.convertTo(disparity32, CV_32F, 1.0 / 16.0);
    return disparity32;
}

vector<Point3f> disparityToPoints(
    const Mat& disparity,
    double focalLength,
    double centerX,
    double centerY,
    double baseline
) {
    vector<Point3f> points;
    points.reserve(static_cast<size_t>(disparity.rows) * disparity.cols / 4);

    for (int y = 0; y < disparity.rows; ++y) {
        for (int x = 0; x < disparity.cols; ++x) {
            const float d = disparity.at<float>(y, x);
            if (!std::isfinite(d) || d <= 10.0f) {
                continue;
            }

            const double z = baseline * focalLength / d;
            const double pointX = (x - centerX) * z / focalLength;
            const double pointY = (y - centerY) * z / focalLength;

            if (std::isfinite(pointX) && std::isfinite(pointY) &&
                std::isfinite(z) && z > 0.0 && z < 1000.0) {
                points.emplace_back(
                    static_cast<float>(pointX),
                    static_cast<float>(pointY),
                    static_cast<float>(z)
                );
            }
        }
    }
    return points;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0]
             << " <imagen_estereo> <calibracion.yml> <salida.obj>\n";
        return EXIT_FAILURE;
    }

    const Mat stereo = imread(argv[1], IMREAD_COLOR);
    Mat left;
    Mat right;
    if (!splitStereoImage(stereo, left, right)) {
        cerr << "La imagen estereo no es valida.\n";
        return EXIT_FAILURE;
    }

    StereoCalibration calibration;
    if (!readCalibration(argv[2], calibration)) {
        return EXIT_FAILURE;
    }

    Mat leftRectified, rightRectified;
    if (!rectify(
            calibration,
            left,
            right,
            leftRectified,
            rightRectified)) {
        cerr << "No se pudieron rectificar las imagenes.\n";
        return EXIT_FAILURE;
    }

    const double focalLength = calibration.leftK.at<double>(0, 0);
    const double centerX = calibration.leftK.at<double>(0, 2);
    const double centerY = calibration.leftK.at<double>(1, 2);
    const double baseline =
        cv::norm(calibration.translation);

    const Mat disparity = computeDisparity(leftRectified, rightRectified);
    const vector<Point3f> points = disparityToPoints(
        disparity, focalLength, centerX, centerY, baseline
    );

    if (!writeObj(argv[3], points)) {
        cerr << "No se pudo escribir el OBJ: " << argv[3] << "\n";
        return EXIT_FAILURE;
    }

    // El enunciado también pide la reconstrucción a media resolución.
    Mat leftHalf, rightHalf;
    resize(leftRectified, leftHalf, Size(), 0.5, 0.5, INTER_AREA);
    resize(rightRectified, rightHalf, Size(), 0.5, 0.5, INTER_AREA);
    const Mat disparityHalf = computeDisparity(leftHalf, rightHalf);
    const vector<Point3f> pointsHalf = disparityToPoints(
        disparityHalf,
        focalLength / 2.0,
        centerX / 2.0,
        centerY / 2.0,
        baseline
    );

    const string halfOutput = "half_" + string(argv[3]);
    if (!writeObj(halfOutput, pointsHalf)) {
        cerr << "No se pudo escribir el OBJ de media resolucion.\n";
        return EXIT_FAILURE;
    }

    cout << "Puntos 3D a resolucion completa: " << points.size() << "\n";
    cout << "Nube 3D guardada en: " << argv[3] << "\n";
    cout << "Puntos 3D a media resolucion: " << pointsHalf.size() << "\n";
    cout << "Nube 3D guardada en: " << halfOutput << "\n";
    return EXIT_SUCCESS;
}