# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(basic LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/virtualkeyboard/basic")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick OPTIONAL_COMPONENTS Multimedia)
if(QT_FEATURE_static)
    find_package(Qt6 REQUIRED COMPONENTS Svg)
endif()

qt_add_executable(basic
    main.cpp
)

set_target_properties(basic PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(basic PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

if(TARGET Qt::Multimedia)
    # See QTBUG-122607
    target_link_libraries(basic PRIVATE Qt::Multimedia)
endif()

# Resources:
set(demo_resource_files
    "Basic.qml"
    "basic-b2qt.qml"
    "content/AutoScroller.qml"
    "content/FloatingButton_Active.svg"
    "content/FloatingButton_Available.svg"
    "content/FloatingButton_Unavailable.svg"
    "content/HandwritingModeButton.qml"
    "content/TextArea.qml"
    "content/TextField.qml"
)

qt6_add_resources(basic "demo"
    PREFIX
        "/"
    FILES
        ${demo_resource_files}
)

if(QT_FEATURE_static)
    target_link_libraries(basic PRIVATE
        Qt::Svg
    )
    qt_import_plugins(basic
        INCLUDE Qt::QVirtualKeyboardPlugin
    )
endif()

if(ANDROID AND TARGET Qt::Multimedia)
    qt_import_plugins(basic
        INCLUDE Qt::QAndroidMediaPlugin
    )
endif()

if(NOT QT_FEATURE_vkb_desktop)
    target_compile_definitions(basic PUBLIC
        MAIN_QML="basic-b2qt.qml"
    )
endif()

if(QT_FEATURE_vkb_desktop)
    target_compile_definitions(basic PUBLIC
        MAIN_QML="Basic.qml"
    )
endif()

install(TARGETS basic
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
