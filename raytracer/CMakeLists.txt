cmake_minimum_required(VERSION 3.14)
project(raytracer)

if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()

include(FetchContent)
# SOURCE_SUBDIR points at a path with no CMakeLists.txt, so this only downloads
# Eigen's headers without also configuring Eigen's own test/doc build.
FetchContent_Declare(eigen GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git GIT_TAG 3.4.0 GIT_SHALLOW TRUE SOURCE_SUBDIR do-not-configure)
FetchContent_Declare(stb GIT_REPOSITORY https://github.com/nothings/stb.git GIT_TAG master)
FetchContent_Declare(gifh GIT_REPOSITORY https://github.com/charlietangora/gif-h.git GIT_TAG master)
FetchContent_MakeAvailable(eigen stb gifh)

# Project sources
add_executable(${PROJECT_NAME}
	src/main.cpp
	src/utils.h
)

# Eigen for linear algebra, stb and gif-h to export images
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${eigen_SOURCE_DIR}" "${stb_SOURCE_DIR}" "${gifh_SOURCE_DIR}")

# Use C++11 version of the standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# Place the output binary at the root of the build folder
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
