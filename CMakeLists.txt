cmake_minimum_required(VERSION 3.22)
project(Tryst VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tryst_core STATIC
    Source/DSP/AnalogEngine.cpp
    Source/Modulation/ModMatrix.cpp
)

target_include_directories(tryst_core PUBLIC Source)

target_compile_features(tryst_core PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(tryst_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(tryst_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
