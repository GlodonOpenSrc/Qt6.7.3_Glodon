# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from loadproxydevicejob.pro.

#####################################################################
## tst_loadproxydevicejob Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_loadproxydevicejob LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_loadproxydevicejob
    SOURCES
        ../commons/testdevice.h
        ../commons/testdevice.cpp
        ../commons/testdeviceproxy.h
        tst_loadproxydevicejob.cpp
    INCLUDE_DIRECTORIES
        ../../core/common
        ../commons
    LIBRARIES
        Qt::3DCore
        Qt::3DCorePrivate
        Qt::3DInput
        Qt::3DInputPrivate
        Qt::CorePrivate
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:loadproxydevicejob.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_loadproxydevicejob CONDITION QT_FEATURE_private_tests
    SOURCES
        ../../core/common/qbackendnodetester.cpp ../../core/common/qbackendnodetester.h
        ../../core/common/testarbiter.h
)
