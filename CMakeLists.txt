cmake_minimum_required(VERSION 3.20)
project(MeowyLoopsStudio VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ----------------------------------------------------------------------
# Options
# ----------------------------------------------------------------------
option(USE_SYSTEM_LIBS "Use system libraries instead of FetchContent" OFF)

# ----------------------------------------------------------------------
# Find Qt6
# ----------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets Concurrent)
set(QT_LIBS Qt6::Core Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Concurrent)

# ----------------------------------------------------------------------
# Find PortAudio
# ----------------------------------------------------------------------
if(USE_SYSTEM_LIBS)
    find_package(PortAudio REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG v19.7.0
    )
    FetchContent_MakeAvailable(portaudio)
    set(PORTAUDIO_LIB portaudio_static)
endif()

# ----------------------------------------------------------------------
# Find libsndfile
# ----------------------------------------------------------------------
if(USE_SYSTEM_LIBS)
    find_package(sndfile REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        libsndfile
        GIT_REPOSITORY https://github.com/libsndfile/libsndfile.git
        GIT_TAG 1.2.2
    )
    FetchContent_MakeAvailable(libsndfile)
    set(SNDFILE_LIB sndfile)
endif()

# ----------------------------------------------------------------------
# Find RtMidi
# ----------------------------------------------------------------------
if(USE_SYSTEM_LIBS)
    find_package(RtMidi REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        rtmidi
        GIT_REPOSITORY https://github.com/thestk/rtmidi.git
        GIT_TAG 5.0.0
    )
    FetchContent_MakeAvailable(rtmidi)
    set(RTMIDI_LIB rtmidi)
endif()

# ----------------------------------------------------------------------
# VST3 SDK (Downloaded by workflow)
# ----------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/vst3sdk")
    set(VST3_SDK_DIR "${CMAKE_SOURCE_DIR}/libs/vst3sdk")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/VST3_SDK")
    set(VST3_SDK_DIR "${CMAKE_SOURCE_DIR}/VST3_SDK")
else()
    set(VST3_SDK_DIR "${CMAKE_SOURCE_DIR}/libs/vst3sdk")
endif()

if(EXISTS "${VST3_SDK_DIR}")
    set(VST3_FOUND TRUE)
    include_directories(${VST3_SDK_DIR})
endif()

# ----------------------------------------------------------------------
# LV2 SDK (Downloaded by workflow)
# ----------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/lv2")
    set(LV2_SDK_DIR "${CMAKE_SOURCE_DIR}/libs/lv2")
else()
    set(LV2_SDK_DIR "${CMAKE_SOURCE_DIR}/libs/lv2")
endif()

if(EXISTS "${LV2_SDK_DIR}")
    set(LV2_FOUND TRUE)
    include_directories(${LV2_SDK_DIR})
endif()

# ----------------------------------------------------------------------
# CLAP SDK (Downloaded by workflow)
# ----------------------------------------------------------------------
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/clap")
    set(CLAP_SDK_DIR "${CMAKE_SOURCE_DIR}/libs/clap")
else()
    set(CLAP_SDK_DIR "${CMAKE_SOURCE_DIR}/libs/clap")
endif()

if(EXISTS "${CLAP_SDK_DIR}")
    set(CLAP_FOUND TRUE)
    include_directories(${CLAP_SDK_DIR})
endif()

# ----------------------------------------------------------------------
# Source Files
# ----------------------------------------------------------------------
set(SOURCES
    src/main.cpp

    # Core
    src/core/engine/AudioEngine.cpp
    src/core/track/Track.cpp
    src/core/track/Clip.cpp
    src/core/mixer/Mixer.cpp
    src/core/transport/Transport.cpp

    # Audio
    src/audio/io/AudioIO.cpp
    src/audio/midi/MIDI.cpp
    src/audio/processing/FX.cpp
    src/audio/processing/EQ.cpp
    src/audio/processing/Compressor.cpp
    src/audio/processing/Reverb.cpp
    src/audio/processing/Delay.cpp
    src/audio/processing/Filter.cpp

    # Plugin Host
    src/plugins/host/PluginHost.cpp
    src/plugins/host/vst3/VST3Host.cpp
    src/plugins/host/lv2/LV2Host.cpp
    src/plugins/host/clap/CLAPHost.cpp

    # UI
    src/ui/mainwindow/MainWindow.cpp
    src/ui/timeline/TimelineWidget.cpp
    src/ui/mixer/MixerWidget.cpp
    src/ui/transport/TransportWidget.cpp
    src/ui/pluginbrowser/PluginBrowserWidget.cpp
    src/ui/pianoroll/PianoRollWidget.cpp

    # Dialogs
    src/dialogs/PreferencesDialog.cpp
    src/dialogs/ExportDialog.cpp

    # Project
    src/project/Project.cpp
    src/project/ProjectLoader.cpp
    src/project/ProjectSaver.cpp

    # Utils
    src/utils/MathUtils.cpp
    src/utils/FileUtils.cpp
)

# ----------------------------------------------------------------------
# Headers
# ----------------------------------------------------------------------
set(HEADERS
    src/core/engine/AudioEngine.h
    src/core/track/Track.h
    src/core/track/Clip.h
    src/core/mixer/Mixer.h
    src/core/transport/Transport.h
    src/audio/io/AudioIO.h
    src/audio/midi/MIDI.h
    src/audio/processing/FX.h
    src/audio/processing/EQ.h
    src/audio/processing/Compressor.h
    src/audio/processing/Reverb.h
    src/audio/processing/Delay.h
    src/audio/processing/Filter.h
    src/plugins/host/PluginHost.h
    src/plugins/host/vst3/VST3Host.h
    src/plugins/host/lv2/LV2Host.h
    src/plugins/host/clap/CLAPHost.h
    src/ui/mainwindow/MainWindow.h
    src/ui/timeline/TimelineWidget.h
    src/ui/mixer/MixerWidget.h
    src/ui/transport/TransportWidget.h
    src/ui/pluginbrowser/PluginBrowserWidget.h
    src/ui/pianoroll/PianoRollWidget.h
    src/dialogs/PreferencesDialog.h
    src/dialogs/ExportDialog.h
    src/project/Project.h
    src/project/ProjectLoader.h
    src/project/ProjectSaver.h
    src/utils/MathUtils.h
    src/utils/FileUtils.h
)

# ----------------------------------------------------------------------
# Include Directories
# ----------------------------------------------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/core/engine
    ${CMAKE_SOURCE_DIR}/src/core/track
    ${CMAKE_SOURCE_DIR}/src/core/mixer
    ${CMAKE_SOURCE_DIR}/src/core/transport
    ${CMAKE_SOURCE_DIR}/src/audio
    ${CMAKE_SOURCE_DIR}/src/audio/io
    ${CMAKE_SOURCE_DIR}/src/audio/midi
    ${CMAKE_SOURCE_DIR}/src/audio/processing
    ${CMAKE_SOURCE_DIR}/src/plugins/host
    ${CMAKE_SOURCE_DIR}/src/plugins/host/vst3
    ${CMAKE_SOURCE_DIR}/src/plugins/host/lv2
    ${CMAKE_SOURCE_DIR}/src/plugins/host/clap
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/dialogs
    ${CMAKE_SOURCE_DIR}/src/project
    ${CMAKE_SOURCE_DIR}/src/utils
)

# ----------------------------------------------------------------------
# Executable
# ----------------------------------------------------------------------
add_executable(MeowyLoopsStudio ${SOURCES} ${HEADERS})

target_link_libraries(MeowyLoopsStudio
    ${QT_LIBS}
    ${PORTAUDIO_LIB}
    ${SNDFILE_LIB}
    ${RTMIDI_LIB}
)

# ----------------------------------------------------------------------
# Windows Specific
# ----------------------------------------------------------------------
if(WIN32)
    target_compile_definitions(MeowyLoopsStudio PRIVATE _WIN32)
    target_link_libraries(MeowyLoopsStudio
        ws2_32
        winmm
        ole32
        oleaut32
        user32
        gdi32
        comctl32
    )
endif()

# ----------------------------------------------------------------------
# macOS Specific
# ----------------------------------------------------------------------
if(APPLE)
    target_link_libraries(MeowyLoopsStudio
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreMIDI"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

# ----------------------------------------------------------------------
# Linux Specific
# ----------------------------------------------------------------------
if(UNIX AND NOT APPLE)
    target_link_libraries(MeowyLoopsStudio
        asound
        pthread
        dl
        m
    )
endif()

# ----------------------------------------------------------------------
# Install
# ----------------------------------------------------------------------
install(TARGETS MeowyLoopsStudio DESTINATION bin)
