# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(audiodecoder)
add_subdirectory(devices)
add_subdirectory(mediaformats)
add_subdirectory(minimal-audio-recorder)
add_subdirectory(minimal-player)
add_subdirectory(wasm)

if(QT_FEATURE_gstreamer)
    add_subdirectory(gstreamer-custom-camera)
    add_subdirectory(gstreamer-custom-camera-rtp)
endif()

if(TARGET Qt::Quick)
    add_subdirectory(qml-minimal-camera)
    add_subdirectory(qml-minimal-player)

    if(QT_FEATURE_gstreamer)
        add_subdirectory(qml-gstreamer-rtp)
    endif()
endif()
