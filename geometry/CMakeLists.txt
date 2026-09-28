cmake_minimum_required(VERSION 3.14)
project(geometry)

if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()

include(FetchContent)
# SOURCE_SUBDIR points at a path with no CMakeLists.txt, so this only downloads
# Eigen's headers without also configuring Eigen's own test/doc build.
FetchContent_Declare(eigen GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git GIT_TAG 3.4.0 GIT_SHALLOW TRUE SOURCE_SUBDIR do-not-configure)
FetchContent_MakeAvailable(eigen)

# Project sources
add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC "${eigen_SOURCE_DIR}")

# Use C++11 version of the standard
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

# Place the output binary at the root of the build folder
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Folder where data files are stored (meshes & stuff)
set(DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/")
target_compile_definitions(${PROJECT_NAME} PUBLIC -DDATA_DIR=\"${DATA_DIR}\")
