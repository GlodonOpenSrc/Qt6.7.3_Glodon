# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qvideoframebackend Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND testdata_resource_files ${test_data_glob})

qt_internal_add_test(tst_qvideoframebackend
    SOURCES
        ../shared/mediafileselector.h
        ../shared/testvideosink.h
        tst_qvideoframebackend.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
    BUILTIN_TESTDATA
    TESTDATA ${testdata_resource_files}
    INCLUDE_DIRECTORIES
        ../shared/
)
