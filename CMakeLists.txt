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

if(WIN32)
    FetchContent_Declare(
        WebView2
        URL https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/1.0.2903.40/microsoft.web.webview2.1.0.2903.40.nupkg
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(WebView2)
endif()

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

if(WIN32)
    target_include_directories(StudioBridgeBalance PRIVATE
        "${webview2_SOURCE_DIR}/build/native/include"
    )
    target_link_libraries(StudioBridgeBalance PRIVATE
        "${webview2_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib"
    )
endif()

target_compile_definitions(StudioBridgeBalance PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

if(WIN32)
    target_compile_definitions(StudioBridgeBalance PRIVATE
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    )
endif()
