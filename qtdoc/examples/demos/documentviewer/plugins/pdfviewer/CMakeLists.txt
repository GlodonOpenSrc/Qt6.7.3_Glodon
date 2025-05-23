# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets PdfWidgets
                 OPTIONAL_COMPONENTS PrintSupport)

qt_add_plugin(pdfviewer
    CLASS_NAME PdfViewer
    pdfviewer.cpp pdfviewer.h
    zoomselector.cpp zoomselector.h
    hoverwatcher.cpp hoverwatcher.h
)

set_target_properties(pdfviewer PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app"
)

target_include_directories(pdfviewer PRIVATE
    ../../app
    ..
)

target_link_libraries(pdfviewer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::PdfWidgets
    abstractviewer
)

if(TARGET Qt6::PrintSupport)
    target_link_libraries(pdfviewer PRIVATE Qt6::PrintSupport)
endif()

if(WIN32)
    set(install_destination "${CMAKE_INSTALL_BINDIR}/app")
elseif(APPLE)
    set(install_destination ".")
else()
    set(install_destination "${CMAKE_INSTALL_BINDIR}")
endif()
install(TARGETS pdfviewer
    RUNTIME DESTINATION "${install_destination}"
    LIBRARY DESTINATION "${install_destination}"
)
