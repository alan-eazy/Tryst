cmake_minimum_required(VERSION 3.22)
project(Tryst VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tryst_core STATIC
    Source/Core/ParameterRegistry.cpp
    Source/DSP/AnalogEngine.cpp
    Source/DSP/EngineFactory.cpp
    Source/DSP/FilterChain.cpp
    Source/DSP/Voice.cpp
    Source/DSP/VoiceManager.cpp
    Source/DSP/StateVariableFilter.cpp
    Source/DSP/WavetableData.cpp
    Source/DSP/WavetableEngine.cpp
    Source/Modulation/AdsrEnvelope.cpp
    Source/Modulation/Lfo.cpp
    Source/Modulation/ModMatrix.cpp
)

target_include_directories(tryst_core PUBLIC Source)
target_compile_features(tryst_core PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(tryst_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(tryst_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(TRYST_BUILD_TESTS "Build Tryst DSP tests" ON)
if (TRYST_BUILD_TESTS)
    enable_testing()
    add_executable(tryst_core_tests Tests/CoreTests.cpp)
    target_link_libraries(tryst_core_tests PRIVATE tryst_core)
    add_test(NAME tryst_core_tests COMMAND tryst_core_tests)
endif()
