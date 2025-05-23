# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from manual.pro.

if(NOT QT_FEATURE_qt3d_extras)
    return()
endif()

add_subdirectory(bigscene-cpp)
add_subdirectory(component-changes)
add_subdirectory(custom-mesh-cpp)
add_subdirectory(custom-mesh-cpp-indirect)
add_subdirectory(custom-mesh-update-data-cpp)
add_subdirectory(cylinder-cpp)
add_subdirectory(cylinder-parent-test)
add_subdirectory(deferred-renderer-cpp)
add_subdirectory(raster-cpp)
add_subdirectory(rendertarget-cpp)
add_subdirectory(qtbug-72236)
add_subdirectory(manual-renderloop)
add_subdirectory(rhi)
add_subdirectory(boundingvolumes)
if(TARGET Qt::Multimedia)
#    add_subdirectory(sharedtexture)
endif()
if(TARGET Qt::Multimedia AND TARGET Qt::Quick)
#    add_subdirectory(sharedtextureqml)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(assimp-cpp)
    add_subdirectory(paintedtexture-cpp)
    add_subdirectory(rendercapture-cpp)
    add_subdirectory(texture-updates-cpp)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(assimp)
    add_subdirectory(animation-keyframe-simple)
    add_subdirectory(animation-keyframe-blendtree)
    add_subdirectory(animation-keyframe-programmatic)
    add_subdirectory(bigmodel-qml)
    add_subdirectory(bigscene-instanced-qml)
    add_subdirectory(clip-planes-qml)
    add_subdirectory(custom-mesh-qml)
    add_subdirectory(custom-mesh-update-data-qml)
    add_subdirectory(cylinder-qml)
    add_subdirectory(deferred-renderer-qml)
    add_subdirectory(downloading)
    add_subdirectory(dynamicscene-cpp)
    add_subdirectory(dragging)
    add_subdirectory(enabled-qml)
    add_subdirectory(gltf)
    add_subdirectory(gooch-qml)
    add_subdirectory(keyboardinput-qml)
    add_subdirectory(lod)
    add_subdirectory(loader-qml)
    add_subdirectory(mouseinput-qml)
    add_subdirectory(multiplewindows-qml)
    add_subdirectory(plasma)
    add_subdirectory(pointlinesize)
    add_subdirectory(scene3d-loader)
    add_subdirectory(picking-qml)
    add_subdirectory(skybox)
    add_subdirectory(simple-shaders-qml)
    add_subdirectory(transparency-qml)
    add_subdirectory(transparency-qml-scene3d)
    add_subdirectory(rendercapture-qml)
    add_subdirectory(additional-attributes-qml)
    add_subdirectory(dynamic-model-loader-qml)
    add_subdirectory(buffercapture-qml)
    add_subdirectory(render-qml-to-texture)
    add_subdirectory(render-qml-to-texture-qml)
    add_subdirectory(transforms-qml)
    add_subdirectory(layerfilter-qml)
    add_subdirectory(tessellation-modes)
    add_subdirectory(rendercapture-qml-fbo)
    add_subdirectory(blitframebuffer-qml)
    add_subdirectory(raycasting-qml)
    add_subdirectory(raster-qml)
    add_subdirectory(shader-image-qml)
    add_subdirectory(spritegrid)
    add_subdirectory(subtree-enabler-qml)
    add_subdirectory(distancefieldtext)
    add_subdirectory(distancefieldtext-dynamic)
    add_subdirectory(mesh-morphing)
    add_subdirectory(anim-viewer)
    add_subdirectory(skinned-mesh)
    add_subdirectory(rigged-simple)
    add_subdirectory(proximityfilter)
    add_subdirectory(scene3d-visibility)
    add_subdirectory(shared_texture_image)
    add_subdirectory(texture_property_updates)
    add_subdirectory(qtbug-76766)
    add_subdirectory(scene3d-in-sync)
    add_subdirectory(compressed_textures)
    add_subdirectory(wave)
    add_subdirectory(advancedcustommaterial)
    add_subdirectory(planets-qml)
    add_subdirectory(compute-particles)
    add_subdirectory(scene3d)

    # these require qt6_add_big_resources but it's broken at this point QTBUG-85051
    # add_subdirectory(anaglyph-rendering)
    # add_subdirectory(qardboard)
    # add_subdirectory(shadow-map-qml)
    # add_subdirectory(phong-cubes)
endif()
if(TARGET Qt::QuickWidgets AND TARGET Qt::Widgets)
    add_subdirectory(widgets-scene3d)
endif()
if(TARGET Qt::Quick AND NOT MACOS AND NOT UIKIT)
    add_subdirectory(compute-manual)
endif()
if(TARGET Qt::QuickWidgets)
    add_subdirectory(quickwidget-switch)
endif()
if(TARGET Qt::Quick AND TARGET Qt::QuickWidgets)
    add_subdirectory(multi-qquickwidgets-scene3d)
endif()
if(TARGET Qt::Multimedia AND TARGET Qt::Quick)
    add_subdirectory(audio-visualizer-qml)
endif()
