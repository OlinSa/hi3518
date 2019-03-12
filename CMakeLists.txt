if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "
    FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.
    ")
endif()

cmake_minimum_required (VERSION 2.8)
project(hi3518)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(Tutorial_VERSION_MAJOR 1)
set(Tutorial_VERSION_MINOR 0)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "set build type to debug")
set(CMAKE_VERBOSE_MAKEFILE on)

ADD_DEFINITIONS("-Wall -g -std=c++11")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)


set(live555_PROVIDER "module" CACHE STRING "Provider of live555 library")

# add_subdirectory(package)