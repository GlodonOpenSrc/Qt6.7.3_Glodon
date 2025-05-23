# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(Magic8Ball LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/grpc/magic8ball")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Protobuf
    Grpc
    GrpcQuick
    Quick
)

qt_standard_project_setup()

add_subdirectory(grpc_server_example)

qt_add_executable(magic8ball
    main.cpp
)

qt_add_protobuf(magic8ball_plugin
    PROTO_FILES
        proto/exampleservice.proto
    QML
)

qt_add_grpc(magic8ball_plugin CLIENT
    PROTO_FILES
        proto/exampleservice.proto
    QML
)

set_target_properties(magic8ball PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

qt_add_qml_module(magic8ball
    URI qtgrpc.examples.magic8ball
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        "WaitingAnimation.qml"
        "AnimatedAnswer.qml"
        "MagicText.qml"
        "ProgressDot.qml"
        "Main.qml"
)

target_link_libraries(magic8ball PRIVATE
    Qt6::Core
    Qt6::Quick
    Qt6::Protobuf
    Qt6::Grpc
    Qt6::GrpcQuick
    magic8ball_plugin
)

install(TARGETS magic8ball
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
