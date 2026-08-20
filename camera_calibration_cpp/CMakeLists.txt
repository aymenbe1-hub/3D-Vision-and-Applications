cmake_minimum_required(VERSION 3.5)
project(CameraCalibration3D)

set(CMAKE_CXX_STANDARD 14)

# Find OpenCV (must be installed: sudo apt install libopencv-dev  OR  brew install opencv)
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# ── calibrate ────────────────────────────────────────────────────────────────
add_executable(calibrate calibrate.cpp)
target_link_libraries(calibrate ${OpenCV_LIBS})

# ── augReal ──────────────────────────────────────────────────────────────────
add_executable(augReal augReal.cpp)
target_link_libraries(augReal ${OpenCV_LIBS})
