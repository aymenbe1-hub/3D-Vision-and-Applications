/**
 * calibrate.cpp
 * =============
 * Calibrates a camera from a set of chessboard images and saves the
 * intrinsic parameters to a YAML file.
 *
 * Usage:
 *   calibrate <board_width> <board_height> <images_folder> [output.yml]
 *
 *   board_width   : number of inner corners horizontally (e.g. 7)
 *   board_height  : number of inner corners vertically   (e.g. 5)
 *   images_folder : path containing the calibration images
 *   output.yml    : output file (default: calibration.yml)
 *
 * Example:
 *   ./calibrate 7 5 ./calibration calibration.yml
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <glob.h>       // POSIX – available on Linux/macOS

using namespace std;
using namespace cv;

// ─── helper: collect image paths from a folder ───────────────────────────────
static vector<string> getImagePaths(const string &folder)
{
    vector<string> paths;
    vector<string> exts = {"*.jpg","*.JPG","*.jpeg","*.png","*.PNG","*.bmp"};
    for (const auto &ext : exts) {
        glob_t g;
        string pattern = folder + "/" + ext;
        if (glob(pattern.c_str(), GLOB_NOSORT | GLOB_MARK, nullptr, &g) == 0)
            for (size_t i = 0; i < g.gl_pathc; i++)
                paths.push_back(g.gl_pathv[i]);
        globfree(&g);
    }
    sort(paths.begin(), paths.end());
    // Remove duplicates (e.g. *.jpg and *.JPG on case-insensitive FS)
    paths.erase(unique(paths.begin(), paths.end()), paths.end());
    return paths;
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(int argc, char **argv)
{
    if (argc < 4) {
        cerr << "Usage: " << argv[0]
             << " <board_width> <board_height> <images_folder> [output.yml]\n";
        return 1;
    }

    const int   BOARD_W  = atoi(argv[1]);
    const int   BOARD_H  = atoi(argv[2]);
    const string folder  = argv[3];
    const string outYml  = (argc > 4) ? argv[4] : "calibration.yml";

    if (BOARD_W <= 0 || BOARD_H <= 0) {
        cerr << "ERROR: board_width and board_height must be positive integers.\n";
        return 1;
    }
    const Size boardSz(BOARD_W, BOARD_H);

    cout << "Board inner corners : " << BOARD_W << " x " << BOARD_H << "\n";
    cout << "Images folder       : " << folder  << "\n";
    cout << "Output YML          : " << outYml  << "\n\n";

    // Termination criteria for cornerSubPix
    const TermCriteria criteria(
        TermCriteria::EPS + TermCriteria::MAX_ITER, 30, 0.001);

    // 3-D object points: unit squares, z = 0 plane
    vector<Point3f> objp;
    for (int r = 0; r < BOARD_H; r++)
        for (int c = 0; c < BOARD_W; c++)
            objp.push_back(Point3f((float)c, (float)r, 0.f));

    vector<vector<Point3f>> objPoints;   // 3-D points per image
    vector<vector<Point2f>> imgPoints;   // 2-D points per image
    Size imageSize;

    // ── Collect images ────────────────────────────────────────────────────────
    vector<string> files = getImagePaths(folder);
    if (files.empty()) {
        cerr << "ERROR: no images found in '" << folder << "'\n"; return 1; }
    cout << "Found " << files.size() << " images\n\n";

    int good = 0;
    for (const auto &fpath : files)
    {
        Mat frame = imread(fpath);
        if (frame.empty()) {
            cout << "  [SKIP] " << fpath << "  (unreadable)\n"; continue; }

        if (imageSize == Size())
            imageSize = Size(frame.cols, frame.rows);

        Mat gray;
        cvtColor(frame, gray, COLOR_BGR2GRAY);

        vector<Point2f> corners;

        // Step 1 — detect chessboard corners
        bool found = findChessboardCorners(
            gray, boardSz, corners,
            CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_FAST_CHECK | CALIB_CB_NORMALIZE_IMAGE
        );

        cout << "  " << fpath << " ... ";

        if (found) {
            // Step 2 — sub-pixel refinement
            cornerSubPix(gray, corners, Size(11, 11), Size(-1, -1), criteria);

            objPoints.push_back(objp);
            imgPoints.push_back(corners);
            good++;
            cout << "OK\n";

            // Draw and show
            drawChessboardCorners(frame, boardSz, corners, found);
            imshow("Calibration", frame);
            waitKey(300);
        } else {
            cout << "corners NOT found\n";
        }
    }
    destroyAllWindows();

    if (good < 3) {
        cerr << "\nERROR: only " << good
             << " valid image(s). Need at least 3.\n"; return 1; }

    cout << "\nCalibrating with " << good << "/" << files.size()
         << " valid images...\n";

    // Step 3 — calibrate
    Mat cameraMatrix, distCoeffs;
    vector<Mat> rvecs, tvecs;
    double rms = calibrateCamera(
        objPoints, imgPoints, imageSize,
        cameraMatrix, distCoeffs, rvecs, tvecs
    );

    cout << "\ncameraMatrix:\n" << cameraMatrix   << "\n";
    cout << "\ndistCoeffs:\n"   << distCoeffs      << "\n";
    cout << "\nRMS reprojection error: " << rms << " px\n";

    // Save — key names match professor's expected format
    FileStorage fs(outYml, FileStorage::WRITE);
    if (!fs.isOpened()) {
        cerr << "ERROR: cannot write to '" << outYml << "'\n"; return 1; }
    fs << "cameraMatrix"  << cameraMatrix;
    fs << "distCoeffs"    << distCoeffs;
    fs << "board_width"   << BOARD_W;
    fs << "board_height"  << BOARD_H;
    fs << "image_width"   << imageSize.width;
    fs << "image_height"  << imageSize.height;
    fs.release();

    cout << "\nSaved to '" << outYml << "'\n";
    return 0;
}
