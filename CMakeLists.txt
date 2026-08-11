cmake_minimum_required(VERSION 3.22)

project(YuViGlow VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary architectures")
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# libsonare — BPM/beat detection (plan/16-beat-detection-and-loop-pads.md).
# Apache-2.0, chosen specifically over more established GPL beat-tracking
# libraries (BTrack/aubio/Gist) to stay AGPL-3.0-compatible (plan/17). It's
# a large multi-subsystem project (mastering/mixing/FX/etc.) — we only need
# beat/BPM detection (sonare_core's C API), so every unrelated subsystem is
# disabled here to keep the build lean. sonare_core calls into sonare_rt for
# its DSP primitives, but CMake's own dependency ordering puts sonare_rt
# *before* sonare_core on the link line (the opposite of what a plain static
# link needs) — wrapped in LINK_GROUP:RESCAN below so the linker retries
# until both resolve, regardless of order. See
# Source/LibsonareTempoDetector.cpp for the actual API calls used.
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_CLI OFF CACHE BOOL "" FORCE)
set(BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(BUILD_MASTERING OFF CACHE BOOL "" FORCE)
set(BUILD_MIXING OFF CACHE BOOL "" FORCE)
set(BUILD_GRAPH OFF CACHE BOOL "" FORCE)
set(BUILD_FX OFF CACHE BOOL "" FORCE)
set(BUILD_ACOUSTIC_SIM OFF CACHE BOOL "" FORCE)
set(BUILD_VOICE_CHANGER OFF CACHE BOOL "" FORCE)
set(BUILD_ASSIST OFF CACHE BOOL "" FORCE)
set(BUILD_WASM OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    libsonare
    GIT_REPOSITORY https://github.com/libraz/libsonare.git
    GIT_TAG v1.6.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(libsonare)

set(YUVIGLOW_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND YUVIGLOW_FORMATS AU)
endif()

juce_add_plugin(YuViGlow
    COMPANY_NAME "CacheRefresh"
    BUNDLE_ID "com.cacherefresh.yuviglow"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Cref
    PLUGIN_CODE Yvg1
    FORMATS ${YUVIGLOW_FORMATS}
    PRODUCT_NAME "YuVi Glow"
)

juce_generate_juce_header(YuViGlow)

target_sources(YuViGlow
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/MidiDeviceManager.cpp
        Source/LibsonareTempoDetector.cpp
)

target_compile_definitions(YuViGlow
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(YuViGlow
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        "$<LINK_GROUP:RESCAN,sonare_core,sonare_rt>"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
