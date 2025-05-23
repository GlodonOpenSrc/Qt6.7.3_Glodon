# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(EGL)

qt_internal_add_module(QGstreamerMediaPluginImplPrivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        audio/qgstreameraudiodevice.cpp audio/qgstreameraudiodevice_p.h
        audio/qgstreameraudiodecoder.cpp audio/qgstreameraudiodecoder_p.h
        common/qglist_helper_p.h
        common/qgst.cpp common/qgst_p.h
        common/qgst_bus.cpp common/qgst_bus_p.h
        common/qgst_debug.cpp common/qgst_debug_p.h
        common/qgst_handle_types_p.h
        common/qgstappsource.cpp common/qgstappsource_p.h
        common/qgstreameraudioinput.cpp common/qgstreameraudioinput_p.h
        common/qgstreameraudiooutput.cpp common/qgstreameraudiooutput_p.h
        common/qgstreamerbufferprobe.cpp common/qgstreamerbufferprobe_p.h
        common/qgstreamermetadata.cpp common/qgstreamermetadata_p.h
        common/qgstreamermessage_p.h
        common/qgstreamermediaplayer.cpp common/qgstreamermediaplayer_p.h
        common/qgstreamervideooutput.cpp common/qgstreamervideooutput_p.h
        common/qgstreamervideooverlay.cpp common/qgstreamervideooverlay_p.h
        common/qgstreamervideosink.cpp common/qgstreamervideosink_p.h
        common/qgstpipeline.cpp common/qgstpipeline_p.h
        common/qgstutils.cpp common/qgstutils_p.h
        common/qgstvideobuffer.cpp common/qgstvideobuffer_p.h
        common/qgstvideorenderersink.cpp common/qgstvideorenderersink_p.h
        common/qgstsubtitlesink.cpp common/qgstsubtitlesink_p.h
        qgstreamerintegration.cpp qgstreamerintegration_p.h
        qgstreamerformatinfo.cpp qgstreamerformatinfo_p.h
        qgstreamervideodevices.cpp qgstreamervideodevices_p.h
        mediacapture/qgstreamercamera.cpp mediacapture/qgstreamercamera_p.h
        mediacapture/qgstreamerimagecapture.cpp mediacapture/qgstreamerimagecapture_p.h
        mediacapture/qgstreamermediacapture.cpp mediacapture/qgstreamermediacapture_p.h
        mediacapture/qgstreamermediaencoder.cpp mediacapture/qgstreamermediaencoder_p.h
    NO_UNITY_BUILD_SOURCES
        # Conflicts with macros defined in X11.h, and Xlib.h
        common/qgstvideobuffer.cpp
        common/qgstreamervideosink.cpp
    NO_GENERATE_CPP_EXPORTS
    DEFINES
        GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26
    PUBLIC_LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        GStreamer::GStreamer
        GStreamer::App
)

qt_internal_extend_target(QGstreamerMediaPluginImplPrivate CONDITION QT_FEATURE_gstreamer_photography
    PUBLIC_LIBRARIES
        GStreamer::Photography
)
qt_internal_extend_target(QGstreamerMediaPluginImplPrivate CONDITION QT_FEATURE_gstreamer_gl
    PUBLIC_LIBRARIES
        GStreamer::Gl
)
qt_internal_extend_target(QGstreamerMediaPluginImplPrivate CONDITION QT_FEATURE_gstreamer_gl_egl
    LIBRARIES
        GStreamer::GlEgl
        EGL::EGL
)
qt_internal_extend_target(QGstreamerMediaPluginImplPrivate CONDITION QT_FEATURE_gstreamer_gl_wayland
    LIBRARIES
        GStreamer::GlWayland
)
qt_internal_extend_target(QGstreamerMediaPluginImplPrivate CONDITION QT_FEATURE_gstreamer_gl_x11
    LIBRARIES
        GStreamer::GlX11
)

qt_internal_add_plugin(QGstreamerMediaPlugin
    OUTPUT_NAME gstreamermediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qgstreamerplugin.cpp
        gstreamer.json
    LIBRARIES
        Qt::QGstreamerMediaPluginImplPrivate
        Qt::MultimediaPrivate
)
