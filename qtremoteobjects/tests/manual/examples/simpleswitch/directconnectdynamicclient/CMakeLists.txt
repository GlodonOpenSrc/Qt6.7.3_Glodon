# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(tst_directconnectdynamicclient VERSION "${QT_REPO_MODULE_VERSION}" LANGUAGES CXX)

if (ANDROID)
    message(FATAL_ERROR "This project cannot be built on Android.")
endif()

set(CMAKE_AUTOMOC ON)

qt_internal_add_manual_test(tst_directconnectdynamicclient
    SOURCES
        main.cpp
        dynamicclient.cpp
        dynamicclient.h
    LIBRARIES
        Qt::Core
        Qt::RemoteObjects
)

set_target_properties(tst_directconnectdynamicclient PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)
