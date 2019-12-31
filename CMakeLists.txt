cmake_minimum_required(VERSION 3.15)
project(raster)

set(CMAKE_CXX_STANDARD 14)



add_executable(raster main.cpp raster.h raster.cpp error.cpp error.h)

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(raster ${Boost_LIBRARIES})