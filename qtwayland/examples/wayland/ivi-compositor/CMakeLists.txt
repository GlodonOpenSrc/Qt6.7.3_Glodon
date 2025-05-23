# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(ivi-compositor LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/wayland/ivi-compositor")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml)

qt_add_executable(ivi-compositor
    main.cpp
)

set_target_properties(ivi-compositor PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(ivi-compositor PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
)

# Resources:
set(ivi-compositor_resource_files
    "main.qml"
)

qt6_add_resources(ivi-compositor "ivi-compositor"
    PREFIX
        "/"
    FILES
        ${ivi-compositor_resource_files}
)

install(TARGETS ivi-compositor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
