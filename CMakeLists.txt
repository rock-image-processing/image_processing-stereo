# CMakeLists.txt has to be located in the project folder and cmake has to be
# executed from 'project/build' with 'cmake ../'.
cmake_minimum_required(VERSION 2.6)
find_package(Rock)
rock_init(stereo 0.1)

rock_opencv_autodetect(OPENCV_PACKAGE)
rock_find_pkgconfig(base_TYPES REQUIRED "base-types")
rock_export_includedir(${PROJECT_SOURCE_DIR}/src stereo)

set(ROCK_TEST_ENABLED OFF)

rock_standard_layout()


