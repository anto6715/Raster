cmake_minimum_required(VERSION 3.15)
project(raster)

set(CMAKE_CXX_STANDARD 14)

add_executable(raster main.cpp raster.h raster.cpp error.cpp error.h)