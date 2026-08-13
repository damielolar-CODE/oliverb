cmake_minimum_required(VERSION 3.22)

project(Oliverb VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")
endif()

# -----------------------------------------------------------------------------
# JUCE
#
#   By default this downloads JUCE for you the first time you configure.
#   If you already have a copy locally, point at it instead and save the
#   download:   cmake -B build -DWH_JUCE_PATH=/path/to/JUCE
# -----------------------------------------------------------------------------
set(WH_JUCE_PATH "" CACHE PATH "Path to an existing JUCE checkout (optional)")

# Install the built plug-in into the system plug-in folder after every build.
# Handy locally, but CI runners cannot write to those folders, so CI turns it off.
option(WH_COPY_PLUGIN "Copy the plug-in into the system plug-in folder after building" ON)

if(WH_JUCE_PATH)
    message(STATUS "OLIVERB: using local JUCE at ${WH_JUCE_PATH}")
    add_subdirectory(${WH_JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# -----------------------------------------------------------------------------
# The plugin
# -----------------------------------------------------------------------------
juce_add_plugin(Oliverb
    COMPANY_NAME                "Deestech"
    BUNDLE_ID                   "com.deestech.oliverb"
    PLUGIN_MANUFACTURER_CODE    Dees
    PLUGIN_CODE                 Olvb
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "OLIVERB"
    DESCRIPTION                 "Passive high-pass filter, tape echo and spring tank"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${WH_COPY_PLUGIN}
    VST3_CATEGORIES             Fx Filter Delay Reverb
    AU_MAIN_TYPE                kAudioUnitType_Effect
)

target_sources(Oliverb PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/LookAndFeel.cpp
)

target_compile_definitions(Oliverb PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(Oliverb PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# -----------------------------------------------------------------------------
# DSP test harness — plain C++, no JUCE, runs in a second.
#   cmake --build build --target dsp_test && ./build/dsp_test
# -----------------------------------------------------------------------------
add_executable(dsp_test Tests/dsp_test.cpp)
target_compile_features(dsp_test PRIVATE cxx_std_17)
