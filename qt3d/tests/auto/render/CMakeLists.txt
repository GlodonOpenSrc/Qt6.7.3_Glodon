# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from render.pro.

if(QT_FEATURE_private_tests)
    add_subdirectory(armature)
    add_subdirectory(aspect)
    add_subdirectory(attribute)
    add_subdirectory(blitframebuffer)
    add_subdirectory(buffer)
    add_subdirectory(computecommand)
    add_subdirectory(coordinatereader)
    add_subdirectory(ddstextures)
    add_subdirectory(effect)
    add_subdirectory(entity)
    add_subdirectory(filterentitybycomponent)
    add_subdirectory(filterkey)
    add_subdirectory(framegraphnode)
    add_subdirectory(framegraphvisitor)
    add_subdirectory(genericlambdajob)
    add_subdirectory(geometry)
    add_subdirectory(geometryrenderer)
    add_subdirectory(joint)
    add_subdirectory(ktxtextures)
    add_subdirectory(layerfiltering)
    add_subdirectory(levelofdetail)
    add_subdirectory(loadscenejob)
    add_subdirectory(material)
    add_subdirectory(memorybarrier)
    add_subdirectory(meshfunctors)
    add_subdirectory(objectpicker)
    add_subdirectory(parameter)
    add_subdirectory(proximityfilter)
    # TO DO: Investigate why this fails on MinGW
    if (NOT MINGW)
        add_subdirectory(proximityfiltering)
    endif()
    add_subdirectory(qabstractlight)
    add_subdirectory(qabstracttexture)
    add_subdirectory(qabstracttextureimage)
    add_subdirectory(qattribute)
    add_subdirectory(qbuffer)
    add_subdirectory(qcamera)
    add_subdirectory(qcameralens)
    add_subdirectory(qcameraselector)
    add_subdirectory(qclearbuffers)
    add_subdirectory(qcomputecommand)
    add_subdirectory(qdispatchcompute)
    add_subdirectory(qeffect)
    add_subdirectory(qenvironmentlight)
    add_subdirectory(qfilterkey)
    add_subdirectory(qframegraphnode)
    add_subdirectory(qgeometry)
    add_subdirectory(qgeometryrenderer)
    add_subdirectory(qgraphicsapifilter)
    add_subdirectory(qlayerfilter)
    add_subdirectory(qlevelofdetail)
    add_subdirectory(qmemorybarrier)
    add_subdirectory(qmesh)
    add_subdirectory(qobjectpicker)
    add_subdirectory(qparameter)
    add_subdirectory(qproximityfilter)
    add_subdirectory(qraycaster)
    add_subdirectory(qrendercapture)
    add_subdirectory(qrenderpass)
    add_subdirectory(qrenderpassfilter)
    add_subdirectory(qrendersettings)
    add_subdirectory(qrenderstate)
    add_subdirectory(qrenderstateset)
    add_subdirectory(qrendersurfaceselector)
    add_subdirectory(qrendertarget)
    add_subdirectory(qrendertargetoutput)
    add_subdirectory(qrendertargetselector)
    add_subdirectory(qsceneloader)
    add_subdirectory(qscreenraycaster)
    add_subdirectory(qsetfence)
    add_subdirectory(qshaderimage)
    add_subdirectory(qshaderprogram)
    add_subdirectory(qshaderprogrambuilder)
    add_subdirectory(qsharedgltexture)
    add_subdirectory(qsortpolicy)
    add_subdirectory(qtechnique)
    add_subdirectory(qtechniquefilter)
    add_subdirectory(qtexturedataupdate)
    add_subdirectory(qtextureimage)
    add_subdirectory(qtextureimagedata)
    add_subdirectory(qtextureloader)
    add_subdirectory(qviewport)
    add_subdirectory(qwaitfence)
    add_subdirectory(raycaster)
    add_subdirectory(rendercapture)
    add_subdirectory(renderpass)
    add_subdirectory(rendertarget)
    add_subdirectory(sceneloader)
    add_subdirectory(segmentvisitor)
    add_subdirectory(setfence)
    add_subdirectory(shader)
    add_subdirectory(shaderbuilder)
    add_subdirectory(shadergraph)
    add_subdirectory(shaderimage)
    add_subdirectory(skeleton)
    add_subdirectory(sortpolicy)
    add_subdirectory(technique)
    add_subdirectory(texture)
    add_subdirectory(transform)
    add_subdirectory(trianglevisitor)
    add_subdirectory(uniform)
    add_subdirectory(vsyncframeadvanceservice)
    add_subdirectory(waitfence)
    add_subdirectory(qray3d)
    add_subdirectory(raycasting)
    add_subdirectory(triangleboundingvolume)
endif()
if(QT_FEATURE_private_tests AND TARGET Qt::Quick)
    add_subdirectory(raycastingjob)
endif()
if(QT_FEATURE_private_tests AND QT_FEATURE_qt3d_opengl_renderer)
    add_subdirectory(opengl)
endif()
if(QT_FEATURE_private_tests AND QT_FEATURE_qt3d_rhi_renderer)
    add_subdirectory(rhi)
endif()
if(QT_FEATURE_private_tests AND QT_FEATURE_qt3d_extras AND QT_FEATURE_qt3d_opengl_renderer)
    add_subdirectory(qmaterial)
    if (NOT QT_BUILD_STANDALONE_TESTS)
        add_subdirectory(geometryloaders)
    endif()
    add_subdirectory(picking)
    add_subdirectory(gltfplugins)
endif()
if(QT_FEATURE_private_tests AND QT_FEATURE_qt3d_extras AND QT_FEATURE_qt3d_opengl_renderer AND TARGET Qt::Quick)
    add_subdirectory(boundingsphere)
    # TO DO: Investigate why this fails on MinGW
    if (NOT MINGW)
        add_subdirectory(pickboundingvolumejob)
    endif()
    add_subdirectory(updateshaderdatatransformjob)
endif()
if(QT_FEATURE_private_tests AND QT_FEATURE_qt3d_input AND QT_FEATURE_qt3d_opengl_renderer AND TARGET Qt::Quick)
    add_subdirectory(qscene2d)
    add_subdirectory(scene2d)
endif()
if(QT_FEATURE_private_tests AND NOT (CMAKE_OSX_ARCHITECTURES MATCHES ";") AND
        (TEST_architecture_arch STREQUAL i386 OR TEST_architecture_arch STREQUAL x86_64))
    add_subdirectory(alignedresourcesmanagers-sse)
endif()
