# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(chapter6-plugins LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(chapter6-plugins
    main.cpp
)

set_target_properties(chapter6-plugins PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(chapter6-plugins PRIVATE
    Qt6::Qml
    Qt6::Quick
)

qt_add_qml_module(chapter6-plugins
    URI ChartsApp
    QML_FILES App.qml
)

add_subdirectory(Charts)

install(TARGETS chapter6-plugins
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET chapter6-plugins
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})

