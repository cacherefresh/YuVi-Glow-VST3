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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
