# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qaspectengine.pro.

#####################################################################
## tst_qaspectengine Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qaspectengine LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qaspectengine
    SOURCES
        tst_qaspectengine.cpp
    LIBRARIES
        Qt::3DCore
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:qaspectengine.pro:<TRUE>:
# TEMPLATE = "app"
