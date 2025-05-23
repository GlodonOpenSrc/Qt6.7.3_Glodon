# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Graph2D Tests
if(QT_FEATURE_graphs_2d)
    if(TARGET Qt::Quick)
        add_subdirectory(testbed)
        if(WIN32)
            add_subdirectory(qmlbarscatter)
        endif()
    endif()
endif()

if(QT_FEATURE_graphs_3d)
    if(TARGET Qt::Quick)
        if(QT_FEATURE_graphs_3d_bars3d)
            add_subdirectory(qmlbarsrowcolors)
            add_subdirectory(qmllegend)
            if(DEFINED Qt6Multimedia_DIR AND DEFINED Qt6MultimediaWidgets_DIR)
                add_subdirectory(qmlqtquick3dintegration)
            endif()
        endif()
        if(QT_FEATURE_graphs_3d_scatter3d)
            add_subdirectory(qmlcustominput)
            add_subdirectory(qmldynamicdata)
        endif()
        if(QT_FEATURE_graphs_3d_surface3d)
            add_subdirectory(qmlgradient)
            add_subdirectory(qmlheightmap)
            add_subdirectory(qmlsurfacelayers)
            add_subdirectory(qmlvolume)
        endif()
        if(QT_FEATURE_graphs_3d_bars3d AND QT_FEATURE_graphs_3d_surface3d)
            add_subdirectory(qmltheme)
        endif()
        if(QT_FEATURE_graphs_3d_bars3d AND QT_FEATURE_graphs_3d_scatter3d AND QT_FEATURE_graphs_3d_surface3d)
            add_subdirectory(qmlmultitest)
            add_subdirectory(qmlperf)
        endif()
    endif()
    if(NOT ANDROID AND NOT IOS AND NOT WINRT)
        if(QT_FEATURE_graphs_3d_bars3d)
            add_subdirectory(barstest)
            add_subdirectory(minimalbars)
        endif()
        if(QT_FEATURE_graphs_3d_scatter3d)
            add_subdirectory(custominput)
            add_subdirectory(directional)
            #add_subdirectory(galaxy) # TODO: Doesn't compile with cmake - qmake works
            add_subdirectory(minimalscatter)
            add_subdirectory(rotations)
            add_subdirectory(scattertest)
        endif()
        if(QT_FEATURE_graphs_3d_surface3d)
            add_subdirectory(minimalsurface)
            add_subdirectory(surfacetest)
        endif()
        if(QT_FEATURE_graphs_3d_bars3d AND QT_FEATURE_graphs_3d_surface3d)
            add_subdirectory(itemmodel)
            add_subdirectory(itemmodeltest)
        endif()
        if(QT_FEATURE_graphs_3d_bars3d AND QT_FEATURE_graphs_3d_scatter3d AND QT_FEATURE_graphs_3d_surface3d)
            add_subdirectory(multigraphs)
            add_subdirectory(volumetrictest)
        endif()
    endif()
endif()
