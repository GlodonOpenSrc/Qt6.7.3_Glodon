# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(Quick3DHelpersImpl
    URI "QtQuick3D.Helpers.impl"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuick3DHelpersImplPlugin
    PLUGIN_TARGET qtquick3dhelpersimplplugin
    NO_PLUGIN_OPTIONAL
    DEPENDENCIES
        QtQuick3D/auto
    SOURCES
        sceneeffects.cpp sceneeffects_p.h
        qquick3drenderstatsmeshesmodel_p.h qquick3drenderstatsmeshesmodel.cpp
        qquick3drenderstatstexturesmodel_p.h qquick3drenderstatstexturesmodel.cpp
        qquick3drenderstatspassesmodel_p.h qquick3drenderstatspassesmodel.cpp
    QML_FILES
        DepthOfFieldBlur.qml
        SceneEffect.qml
        LightmapperOutputWindow.qml
    LIBRARIES
        Qt::Qml
        Qt::Quick
        Qt::Quick3DPrivate
)

qt_internal_add_resource(Quick3DHelpersImpl "helpersimpl_resources"
    PREFIX
        "/qtquick3d_helpers"
    FILES
        shaders/lensflareblurhorizontal.vert
        shaders/lensflareblurvertical.vert
        shaders/lensflaredownsample.frag
        shaders/lensflarefeatures.frag
        shaders/lensflaregaussianblur.frag
        shaders/tonemapper.frag
        shaders/depthoffieldblur.frag
        shaders/depthoffieldblur.vert
        shaders/downsample.frag
        shaders/downsample.vert
        shaders/glowhorizontalblur.frag
        shaders/glowverticalblur.frag

        images/lens_dirt_default.jpeg
        images/noiseTexture.png
        images/gradientTexture.png

        luts/identity.png
)
