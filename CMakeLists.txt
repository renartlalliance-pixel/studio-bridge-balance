cmake_minimum_required(VERSION 3.22)

project(StudioBridgeBalance VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

if(APPLE)
    set(STUDIO_BRIDGE_PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(STUDIO_BRIDGE_PLUGIN_FORMATS VST3 Standalone)
endif()

juce_add_plugin(StudioBridgeBalance
    COMPANY_NAME "TonNom"
    PLUGIN_MANUFACTURER_CODE Tnom
    PLUGIN_CODE SbBa
    FORMATS ${STUDIO_BRIDGE_PLUGIN_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_NAME "Studio Bridge Balance"
    PRODUCT_NAME "Studio Bridge Balance"
)

target_sources(StudioBridgeBalance PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_link_libraries(StudioBridgeBalance PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

target_compile_definitions(StudioBridgeBalance PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
