# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This is an automatic test for the CMake configuration files.
# To run it manually,
# 1) mkdir build   # Create a build directory
# 2) cd build
# 3) # Run cmake on this directory
#    `$qt_prefix/bin/qt-cmake ..` or `cmake -DCMAKE_PREFIX_PATH=/path/to/qt ..`
# 4) ctest         # Run ctest
# 5) ctest -V -R test_add_translation_macro # Run single test
#
# The expected output is something like:
#
#     Start 1: test_add_translation_macro
# 1/2 Test #1: test_add_translation_macro .......   Passed    3.13 sec
#     Start 2: test_create_translation_macro
# 2/2 Test #2: test_create_translation_macro ....   Passed    3.64 sec
#
#
# Note that if Qt is not installed, or if it is installed to a
# non-standard prefix, the environment variable CMAKE_PREFIX_PATH
# needs to be set to the installation prefix or build prefix of Qt
# before running these tests.

cmake_minimum_required(VERSION 3.16)

project(qttools_cmake_tests_linguist)

enable_testing()

set(required_packages Core LinguistTools)

# Setup the test when called as a completely standalone project.
if(TARGET Qt6::Core)
    # Tests are built as part of the qtbase build tree.
    # Setup paths so that the Qt packages are found, similar to examples.
    qt_internal_set_up_build_dir_package_paths()
endif()

find_package(Qt6 REQUIRED COMPONENTS ${required_packages})

# Setup common test variables which were previously set by ctest_testcase_common.prf.
set(CMAKE_MODULES_UNDER_TEST "${required_packages}")
foreach(qt_package ${optional_packages})
    set(package_name "${QT_CMAKE_EXPORT_NAMESPACE}${qt_package}")
    list(APPEND CMAKE_MODULES_UNDER_TEST "${qt_package}")
endforeach()

foreach(qt_package ${CMAKE_MODULES_UNDER_TEST})
    if(${package_name}_FOUND)
        set(CMAKE_${qt_package}_MODULE_MAJOR_VERSION "${${package_name}_VERSION_MAJOR}")
        set(CMAKE_${qt_package}_MODULE_MINOR_VERSION "${${package_name}_VERSION_MINOR}")
        set(CMAKE_${qt_package}_MODULE_PATCH_VERSION "${${package_name}_VERSION_PATCH}")
    endif()
endforeach()

include("${_Qt6CTestMacros}")

_qt_internal_test_expect_pass(test_add_translation_macro)
_qt_internal_test_expect_pass(test_create_translation_macro)
_qt_internal_test_expect_pass(test_create_translation_same_base_names)
_qt_internal_test_expect_pass(test_translation_api)
_qt_internal_test_expect_pass(test_i18n_auto_ts_file_names)
_qt_internal_test_expect_pass(test_i18n_exclusion)
_qt_internal_test_expect_pass(test_i18n_filter_autogen_files)
_qt_internal_test_expect_pass(test_i18n_source_language)
if(NOT CMAKE_CROSSCOMPILING)
    _qt_internal_test_expect_pass(test_i18n_find_package_in_subdir BINARY app1)
    _qt_internal_test_expect_pass(test_i18n_subdir BINARY app1)
endif()
