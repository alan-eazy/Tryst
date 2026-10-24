cmake_minimum_required(VERSION 3.22)
project(Tryst VERSION 0.14.0 LANGUAGES CXX)

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
    Source/FX/DelayEffect.cpp
    Source/FX/DriveEffect.cpp
    Source/FX/EffectFactory.cpp
    Source/FX/FxRack.cpp
    Source/Modulation/AdsrEnvelope.cpp
    Source/Modulation/FastModulationRuntime.cpp
    Source/Modulation/Lfo.cpp
    Source/Modulation/Mseg.cpp
    Source/Modulation/ModMatrix.cpp
    Source/Modulation/ModulationRuntime.cpp
    Source/Modulation/StepSequencer.cpp
    Source/State/PatchApplier.cpp
    Source/State/PatchCodec.cpp
    Source/Tuning/TuningTable.cpp
)

target_include_directories(tryst_core PUBLIC Source)
target_compile_features(tryst_core PUBLIC cxx_std_20)

add_library(tryst_plugin_engine STATIC
    Source/Plugin/HostParameterCatalog.cpp
    Source/Plugin/PluginEngine.cpp
)
target_include_directories(tryst_plugin_engine PUBLIC Source)
target_link_libraries(tryst_plugin_engine PUBLIC tryst_core)
target_compile_features(tryst_plugin_engine PUBLIC cxx_std_20)

if (MSVC)
    target_compile_options(tryst_core PRIVATE /W4 /permissive-)
    target_compile_options(tryst_plugin_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(tryst_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tryst_plugin_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(TRYST_BUILD_PLUGINS "Build Tryst CLAP and VST3 plugin targets" OFF)
if (TRYST_BUILD_PLUGINS)
    include(FetchContent)

    set(CLAP_WRAPPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)
    set(CLAP_SUPPORTS_ALL_NOTE_EXPRESSIONS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        clap_wrapper
        GIT_REPOSITORY https://github.com/free-audio/clap-wrapper.git
        GIT_TAG v0.16.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(clap_wrapper)

    set_target_properties(tryst_core tryst_plugin_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

    add_library(tryst_clap_impl STATIC
        Source/Plugin/Clap/TrystClap.cpp
    )
    target_include_directories(tryst_clap_impl PUBLIC Source Source/Plugin/Clap)
    target_link_libraries(tryst_clap_impl PUBLIC tryst_plugin_engine clap clap-wrapper-extensions)
    target_compile_features(tryst_clap_impl PUBLIC cxx_std_20)
    target_compile_definitions(tryst_clap_impl PRIVATE TRYST_PLUGIN_VERSION="${PROJECT_VERSION}")
    set_target_properties(tryst_clap_impl PROPERTIES POSITION_INDEPENDENT_CODE ON)

    if (MSVC)
        target_compile_options(tryst_clap_impl PRIVATE /W4 /permissive-)
    else()
        target_compile_options(tryst_clap_impl PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    make_clapfirst_plugins(
        TARGET_NAME tryst_formats
        IMPL_TARGET tryst_clap_impl
        OUTPUT_NAME "Tryst"
        ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/Source/Plugin/Clap/TrystClapExport.cpp"
        BUNDLE_IDENTIFIER "com.alan-eazy.tryst"
        BUNDLE_VERSION ${PROJECT_VERSION}
        COPY_AFTER_BUILD FALSE
        WINDOWS_FOLDER_VST3 TRUE
        PLUGIN_FORMATS CLAP VST3
        ASSET_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Tryst_assets"
    )
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

    add_executable(tryst_expression_tuning_tests Tests/ExpressionTuningTests.cpp)
    target_link_libraries(tryst_expression_tuning_tests PRIVATE tryst_core)
    add_test(NAME tryst_expression_tuning_tests COMMAND tryst_expression_tuning_tests)

    add_executable(tryst_fx_rack_tests Tests/FxRackTests.cpp)
    target_link_libraries(tryst_fx_rack_tests PRIVATE tryst_core)
    add_test(NAME tryst_fx_rack_tests COMMAND tryst_fx_rack_tests)

    add_executable(tryst_plugin_engine_tests Tests/PluginEngineTests.cpp)
    target_link_libraries(tryst_plugin_engine_tests PRIVATE tryst_plugin_engine)
    add_test(NAME tryst_plugin_engine_tests COMMAND tryst_plugin_engine_tests)
endif()
