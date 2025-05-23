# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qtmultimedia.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtMultimedia
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Multimedia Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG OPTIONAL_COMPONENTS Gui Network Svg Widgets Quick Qml QuickTest QuickControls2 Quick3D)
qt_internal_project_setup()

if(NOT TARGET Qt::Gui)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Gui\" is not met.")
    return()
endif()
if(NOT TARGET Qt::Network)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Network\" is not met.")
    return()
endif()

set(QT_BUILD_EXTRA_IDE_FILE_PATTERNS
    src/multimedia/doc/snippets/multimedia-snippets/images/qt-logo.png
    src/multimedia/doc/snippets/multimedia-snippets/devices.cpp
    src/multimedia/doc/snippets/multimedia-snippets/qsound.cpp
    src/multimedia/doc/snippets/multimedia-snippets/audio.cpp
    src/multimedia/doc/snippets/multimedia-snippets/camera.cpp
    src/multimedia/doc/snippets/multimedia-snippets/media.cpp
    src/multimedia/doc/snippets/multimedia-snippets/multiple-videooutputs.qml
    src/multimedia/doc/snippets/multimedia-snippets/qtvideosink.qml
    src/multimedia/doc/snippets/multimedia-snippets/soundeffect.qml
    src/multimedia/doc/snippets/multimedia-snippets/video.cpp
    src/multimedia/doc/snippets/CMakeLists.txt
    src/multimedia/doc/src/classic.css
    src/multimedia/doc/src/examples/video-qml-paint-rate.qdocinc
    src/multimedia/doc/src/images/annotatedurl.png
    src/multimedia/doc/src/images/codeless.png
    src/multimedia/doc/src/images/qmlcamera-menu.png
    src/multimedia/doc/src/images/radio-example.png
    src/multimedia/doc/src/images/slideshow-img1.png
    src/multimedia/doc/src/images/video-graphics-memory.png
    src/multimedia/doc/src/images/video-qml-paint-rate.png
    src/multimedia/doc/src/images/Zoom.gif
    src/multimedia/doc/src/images/how-focus-works.gif
    src/multimedia/doc/src/images/image_processing.png
    src/multimedia/doc/src/images/noun_Media_166644.svg
    src/multimedia/doc/src/images/qS1FmgPVL.jpg
    src/multimedia/doc/src/images/sound-wave-small.jpg
    src/multimedia/doc/src/images/camera_correctionAngle_90.png
    src/multimedia/doc/src/qt6-changes.qdoc
    src/multimedia/doc/src/qtmultimedia-cpp.qdoc
    src/multimedia/doc/src/qtmultimedia-examples.qdoc
    src/multimedia/doc/src/qtmultimedia-qml-types.qdoc
    src/multimedia/doc/src/videooverview.qdoc
    src/multimedia/doc/src/audiooverview.qdoc
    src/multimedia/doc/src/cameraoverview.qdoc
    src/multimedia/doc/src/multimedia-overview.qdoc
    src/multimedia/doc/src/platform-notes-apple.qdoc
    src/multimedia/doc/src/platform-notes-wasm.qdoc
    src/multimedia/doc/src/qm-external-pages.qdoc
    src/multimedia/doc/src/qtmultimedia-building-from-source.qdoc
    src/multimedia/doc/src/qtmultimedia-index.qdoc
    src/multimedia/doc/QtMultimediaDoc
    src/multimedia/doc/qtmultimedia.qdocconf
)

qt_build_repo()
