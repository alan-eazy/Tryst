cmake_minimum_required(VERSION 3.22)
project(Tryst VERSION 0.11.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(tryst_core STATIC
    Source/Core/ParameterRegistry.cpp
    Source/DSP/AnalogEngine.cpp
    Source/DSP/EngineFactory.cpp
    Source/DSP/FilterChain.cpp
    Source/DSP/GranularEngine.cpp
    Source/DSP/NoiseEngine.cpp
    Source/DSP/SampleData.cpp
    Source/DSP/SamplerEngine.cpp
    Source/DSP/Voice.cpp
    Source/DSP/VoiceManager.cpp
    Source/DSP/StateVariableFilter.cpp
    Source/DSP/WavetableData.cpp
    Source/DSP/WavetableEngine.cpp
    Source/Modulation/AdsrEnvelope.cpp
    Source/Modulation/FastModulationRuntime.cpp
    Source/Modulation/Lfo.cpp
    Source/Modulation/Mseg.cpp
    Source/Modulation/ModMatrix.cpp
    Source/Modulation/ModulationRuntime.cpp
    Source/Modulation/StepSequencer.cpp
    Source/State/PatchApplier.cpp
    Source/State/PatchCodec.cpp
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

    add_executable(tryst_granular_tests Tests/GranularTests.cpp)
    target_link_libraries(tryst_granular_tests PRIVATE tryst_core)
    add_test(NAME tryst_granular_tests COMMAND tryst_granular_tests)

    add_executable(tryst_fast_modulation_tests Tests/FastModulationTests.cpp)
    target_link_libraries(tryst_fast_modulation_tests PRIVATE tryst_core)
    add_test(NAME tryst_fast_modulation_tests COMMAND tryst_fast_modulation_tests)

    add_executable(tryst_patch_state_tests Tests/PatchStateTests.cpp)
    target_link_libraries(tryst_patch_state_tests PRIVATE tryst_core)
    add_test(NAME tryst_patch_state_tests COMMAND tryst_patch_state_tests)

    add_executable(tryst_noise_engine_tests Tests/NoiseEngineTests.cpp)
    target_link_libraries(tryst_noise_engine_tests PRIVATE tryst_core)
    add_test(NAME tryst_noise_engine_tests COMMAND tryst_noise_engine_tests)

    add_executable(tryst_shape_modulator_tests Tests/ShapeModulatorTests.cpp)
    target_link_libraries(tryst_shape_modulator_tests PRIVATE tryst_core)
    add_test(NAME tryst_shape_modulator_tests COMMAND tryst_shape_modulator_tests)
endif()
