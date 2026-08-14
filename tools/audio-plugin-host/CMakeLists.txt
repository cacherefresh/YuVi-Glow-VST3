# Standalone build of JUCE's own AudioPluginHost, for verifying YuVi Glow's
# VST3 build loads and works in a real host without needing Serato/Mixxx —
# gitignored, dev-only, not part of the actual YuVi Glow plugin build.
# Reuses the JUCE checkout already fetched for the main project instead of
# re-downloading it.
cmake_minimum_required(VERSION 3.22)
project(YuViGlowAudioPluginHostTool)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_BUILD_EXTRAS ON CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../build/_deps/juce-src" juce_build)

# The stock target enables LADSPA hosting (we only actually need VST3, and
# later LV2), which needs system header ladspa.h (package ladspa-sdk) that
# isn't installed and would need an interactive sudo password to add.
# Redefining JUCE_PLUGINHOST_LADSPA=0 via target_compile_definitions doesn't
# reliably win against the vendored target's own =1 (compiler picked the
# first -D seen, not the last, despite the redefinition warning) — so
# instead we just supply the real header: canonical LADSPA v1.1 (LGPL),
# fetched verbatim from https://github.com/swh/ladspa/blob/master/ladspa.h,
# a long-stable, unchanged reference header, specifically LGPL-licensed to
# permit exactly this kind of free interoperability use. LADSPA hosting
# itself is still never exercised — we just need the type declarations to
# satisfy the compile.
if(TARGET AudioPluginHost)
    target_include_directories(AudioPluginHost PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()
