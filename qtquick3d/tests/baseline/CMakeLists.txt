# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from quick3d_lancelot.pro.

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquick3d LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Special case: test includes the QBaselineTest module sources from qtbase directly
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../../qtbase/tests/baseline/shared")
    add_subdirectory(scenegrabber)
    if(QT_FEATURE_process)
        add_subdirectory(quick3d)
    endif()
endif()
