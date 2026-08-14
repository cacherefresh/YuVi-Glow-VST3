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

# libsonare — BPM/beat detection (plan/issues/16-beat-detection-and-loop-pads.md).
# Apache-2.0, chosen specifically over more established GPL beat-tracking
# libraries (BTrack/aubio/Gist) to stay AGPL-3.0-compatible (plan/issues/17). It's
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

# LINK_GROUP:RESCAN maps onto GNU ld's --start-group/--end-group, which CMake
# only defines for Linux/*BSD/SunOS/Windows-GNU — Platform/Darwin.cmake has no
# CMAKE_LINK_GROUP_USING_RESCAN at all. Using it on macOS is not a silent
# no-op: CMake hard-fails at generate time with "Feature 'RESCAN' ... is not
# supported for the 'CXX' link language", before a single file compiles.
# Apple's ld64 doesn't need the hint anyway — unlike GNU ld's single pass, it
# resolves static archives regardless of command-line order, so plain library
# names give the same result. Verified against CMake 3.28's own platform
# modules, not assumed.
if(APPLE)
    set(YUVIGLOW_SONARE_LINK sonare_core sonare_rt)
else()
    set(YUVIGLOW_SONARE_LINK "$<LINK_GROUP:RESCAN,sonare_core,sonare_rt>")
endif()

set(YUVIGLOW_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND YUVIGLOW_FORMATS AU)
endif()
if(UNIX AND NOT APPLE)
    # LV2 — lets Mixxx host YuVi Glow directly (Mixxx has no VST/VST3
    # support at all, confirmed by inspecting the installed binary, but its
    # installed build does link liblilv and genuinely hosts LV2 plugins via
    # its LV2Backend). JUCE's own CMake tooling has real first-party LV2
    # support (see extras/Build/CMake/JUCEUtils.cmake), so this is a small,
    # precedented addition, not a new subsystem. Scoped to Linux for now,
    # matching where it's actually been verified — see
    # plan/issues/20-mvp-beta.md and the session that added this.
    list(APPEND YUVIGLOW_FORMATS LV2)
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
    # IS_SYNTH FALSE gives this plugin a real stereo *input* bus, so JUCE's
    # StandalonePluginHolder opens an audio input device at startup
    # (audioInputRequired = inChannels > 0 — juce_StandaloneFilterWindow.h).
    # On macOS that counts as microphone access, and TCC kills any process
    # that reaches it without NSMicrophoneUsageDescription in its Info.plist.
    # JUCE only emits that key when MICROPHONE_PERMISSION_ENABLED is TRUE
    # (juce_PlistOptions defaults it to false), and RuntimePermissions is a
    # no-op off Android, so nothing else catches this. Without these two
    # lines the Standalone smoke test crashes on launch on the Mac; it never
    # showed up on Linux because no Info.plist is generated there.
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "YuVi Glow opens an audio input device because it runs as an audio effect. It does not record or transmit anything."
    PLUGIN_MANUFACTURER_CODE Cref
    PLUGIN_CODE Yvg1
    FORMATS ${YUVIGLOW_FORMATS}
    PRODUCT_NAME "YuVi Glow"
    LV2URI "https://github.com/cacherefresh/YuVi-Glow-VST3/lv2/YuViGlow"
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
        # JUCE's registerBasicFormats() only registers MP3 decoding if this
        # is set — without it, formatManager.createReaderFor() silently
        # returns nullptr for any .mp3 (found via real user testing: file
        # picker let you choose one, but nothing loaded, no error, no way to
        # tell why). JUCE's own header carries an explicit disclaimer on this
        # flag ("NOT guaranteed to be free from infringements of 3rd-party
        # intellectual property... at your own risk") — flagged here rather
        # than silently enabled, per this project's licensing ground rules
        # (see CREDITS.md). In practice, the core MP3 patents (Fraunhofer/
        # Technicolor) expired industry-wide around 2017; real risk today is
        # low, and MP3 loading is an explicit project requirement (see
        # assets/WARNING_NON_FREE/).
        JUCE_USE_MP3AUDIOFORMAT=1
)

target_link_libraries(YuViGlow
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        ${YUVIGLOW_SONARE_LINK}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
