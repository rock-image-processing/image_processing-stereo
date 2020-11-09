# include macro
include(FindPkgConfig)

rock_find_pkgconfig(OPENCV REQUIRED ${OPENCV_PACKAGE})
if (${OPENCV_VERSION} VERSION_LESS 2.4)
    message(STATUS "found pre-2.4 OpenCV version")
    # The CV 2.3 pkg-config file does not include the opencv_gpu library. Look for
    # it manually
    find_library(OPENCV_GPU_LIB NAMES opencv_gpu HINTS ${OPENCV_LIBRARY_DIRS})
    if (NOT OPENCV_GPU_LIB)
        message(FATAL_ERROR "cannot find the opencv_gpu library in ${OPENCV_LIBRARY_DIRS}. This is required")
    endif()

    add_definitions(-DOPENCV_HAS_SURF)
    add_definitions(-DOPENCV_HAS_SIFT)
    add_definitions(-DOPENCV_HAS_SURF_GPU)

    set(BUILD_SPARSE_STEREO TRUE)
elseif (${OPENCV_VERSION} VERSION_LESS 3.0)
    message(STATUS "found OpenCV version 2.4x in ${OPENCV_INCLUDE_DIRS}")
    # Check if the non-free stuff is there
    find_file(OPENCV_HAS_NONFREE
        opencv2/nonfree/features2d.hpp
        PATHS ${OPENCV_INCLUDE_DIRS})
    # .. and the gpu
    find_file(OPENCV_HAS_GPU
        opencv2/gpu/gpu.hpp
        PATHS ${OPENCV_INCLUDE_DIRS})

    set(OPENCV_HAS_GPUMAT_IN_CORE TRUE)
    set(PSURF_NEEDS_LEGACY TRUE)
    if (OPENCV_HAS_NONFREE)
        add_definitions(-DOPENCV_HAS_SURF -DOPENCV_HAS_SIFT -DOPENCV_HAS_NONFREE)
    else()
        message(STATUS "the OpenCV version you have does not provide the non-free algorithms such has SURF and SIFT, support for them will be disabled")
    endif()

    # Even if we had the GPU / SURF GPU code, the sparse stereo class uses
    # pre-2.4 API for which there is no backward compatibility
    if (false AND OPENCV_HAS_GPU)
        add_definitions(-DOPENCV_HAS_SURF_GPU)
    else()
        message(STATUS "the OpenCV version you have does not provide the GPU-accelerated algorithms, support for them will be disabled")
    endif()

    set(BUILD_SPARSE_STEREO TRUE)
else()
    message(STATUS "found post-3.0 OpenCV version. This version does not support the current implementation of sparse features, excluding them from builds.")
    set(BUILD_SPARSE_STEREO FALSE)
endif()

configure_file(config.h.in stereo/config.h)
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

# use sse3 instruction set
set(CMAKE_CXX_FLAGS "-msse3")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")

set(stereo_SOURCES densestereo.cpp homography.cpp dense_stereo_types.cpp configuration.cpp ransac.cpp)
set(stereo_HEADERS densestereo.h dense_stereo_types.h ransac.cpp homography.h store_vector.hpp)

if (BUILD_SPARSE_STEREO)
    list(APPEND stereo_SOURCES psurf.cpp sparse_stereo.cpp)
    list(APPEND stereo_HEADERS psurf.h sparse_stereo.hpp sparse_stereo_types.h)
endif()

rock_library(stereo
    SOURCES ${stereo_SOURCES}
    DEPS_PKGCONFIG ${OPENCV_PACKAGE} frame_helper libelas
    HEADERS ${stereo_HEADERS})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/stereo/config.h
    DESTINATION include/stereo)

target_link_libraries(stereo ${OPENCV_GPU_LIB})
