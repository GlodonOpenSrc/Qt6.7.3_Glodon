# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(devices LANGUAGES CXX)

if(ANDROID OR IOS)
    message(FATAL_ERROR "This is a commandline tool that is not supported on mobile platforms")
endif()

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/mediaformats")

find_package(Qt6 REQUIRED COMPONENTS Core Multimedia)

qt_add_executable(mediaformats
    main.cpp
)

set_target_properties(mediaformats PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(mediaformats PUBLIC
    Qt::Core
    Qt::Multimedia
)

install(TARGETS mediaformats
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
