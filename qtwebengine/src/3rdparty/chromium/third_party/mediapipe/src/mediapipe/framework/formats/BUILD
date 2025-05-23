# Copyright 2019 The MediaPipe Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

load("@bazel_skylib//lib:selects.bzl", "selects")
load("//mediapipe/framework/port:build_config.bzl", "mediapipe_proto_library")
load("//mediapipe/framework:mediapipe_register_type.bzl", "mediapipe_register_type")

package(
    default_visibility = ["//visibility:public"],
    features = ["-layering_check"],
)

licenses(["notice"])

selects.config_setting_group(
    name = "ios_or_disable_gpu",
    match_any = [
        "//mediapipe/gpu:disable_gpu",
        "//mediapipe:ios",
    ],
)

mediapipe_proto_library(
    name = "detection_proto",
    srcs = ["detection.proto"],
    deps = [":location_data_proto"],
)

mediapipe_register_type(
    base_name = "detection",
    include_headers = ["mediapipe/framework/formats/detection.pb.h"],
    types = [
        "::mediapipe::Detection",
        "::mediapipe::DetectionList",
        "::std::vector<::mediapipe::Detection>",
        "::std::vector<::mediapipe::DetectionList>",
    ],
    deps = [":detection_cc_proto"],
)

mediapipe_proto_library(
    name = "classification_proto",
    srcs = ["classification.proto"],
)

mediapipe_register_type(
    base_name = "classification",
    include_headers = ["mediapipe/framework/formats/classification.pb.h"],
    types = [
        "::mediapipe::Classification",
        "::mediapipe::ClassificationList",
        "::mediapipe::ClassificationListCollection",
        "::std::vector<::mediapipe::Classification>",
        "::std::vector<::mediapipe::ClassificationList>",
    ],
    deps = [":classification_cc_proto"],
)

mediapipe_proto_library(
    name = "image_format_proto",
    srcs = ["image_format.proto"],
)

mediapipe_proto_library(
    name = "matrix_data_proto",
    srcs = ["matrix_data.proto"],
)

mediapipe_proto_library(
    name = "location_data_proto",
    srcs = ["location_data.proto"],
    deps = ["//mediapipe/framework/formats/annotation:rasterization_proto"],
)

mediapipe_proto_library(
    name = "affine_transform_data_proto",
    srcs = ["affine_transform_data.proto"],
)

mediapipe_proto_library(
    name = "time_series_header_proto",
    srcs = ["time_series_header.proto"],
)

mediapipe_proto_library(
    name = "image_file_properties_proto",
    srcs = ["image_file_properties.proto"],
)

cc_library(
    name = "deleting_file",
    srcs = ["deleting_file.cc"],
    hdrs = ["deleting_file.h"],
    deps = [
        "@com_google_absl//absl/log:absl_log",
    ],
)

cc_library(
    name = "matrix",
    srcs = ["matrix.cc"],
    hdrs = ["matrix.h"],
    deps = [
        ":matrix_data_cc_proto",
        "//mediapipe/framework:port",
        "//mediapipe/framework/port:core_proto",
        "//mediapipe/framework/port:logging",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:status",
        "@com_google_absl//absl/log:absl_check",
        "@eigen_archive//:eigen3",
    ],
)

cc_library(
    name = "affine_transform",
    srcs = ["affine_transform.cc"],
    hdrs = ["affine_transform.h"],
    deps = [
        ":affine_transform_data_cc_proto",
        "//mediapipe/framework:port",
        "//mediapipe/framework:type_map",
        "//mediapipe/framework/port:integral_types",
        "//mediapipe/framework/port:logging",
        "//mediapipe/framework/port:point",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:statusor",
        "//mediapipe/framework/tool:status_util",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "image_frame",
    srcs = ["image_frame.cc"],
    hdrs = ["image_frame.h"],
    deps = [
        ":image_format_cc_proto",
        "//mediapipe/framework:port",
        "//mediapipe/framework/port:aligned_malloc_and_free",
        "//mediapipe/framework/port:core_proto",
        "//mediapipe/framework/port:integral_types",
        "//mediapipe/framework/port:source_location",
        "//mediapipe/framework/tool:type_util",
        "@com_google_absl//absl/base",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ] + select({
        "//conditions:default": [
        ],
        "//mediapipe/framework:disable_rtti_and_exceptions": [],
    }),
)

cc_library(
    name = "image_frame_opencv",
    srcs = ["image_frame_opencv.cc"],
    hdrs = ["image_frame_opencv.h"],
    deps = [
        ":image_format_cc_proto",
        ":image_frame",
        "//mediapipe/framework/port:opencv_core",
    ],
)

config_setting(
    name = "opencv",
    define_values = {
        "use_opencv": "true",
    },
)

config_setting(
    name = "portable_opencv",
    define_values = {
        "use_portable_opencv": "true",
        "use_opencv": "false",
    },
)

cc_library(
    name = "location",
    srcs = ["location.cc"],
    hdrs = ["location.h"],
    deps = [
        ":location_data_cc_proto",
        "//mediapipe/framework:port",
        "//mediapipe/framework:type_map",
        "//mediapipe/framework/formats/annotation:locus_cc_proto",
        "//mediapipe/framework/formats/annotation:rasterization_cc_proto",
        "//mediapipe/framework/port:integral_types",
        "//mediapipe/framework/port:point",
        "//mediapipe/framework/port:rectangle",
        "//mediapipe/framework/port:ret_check",
        "//mediapipe/framework/port:status",
        "//mediapipe/framework/port:statusor",
        "//mediapipe/framework/tool:status_util",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ] + select({
        "//conditions:default": [
        ],
        "//mediapipe:android": [],
        "//mediapipe:ios": [],
    }),
    alwayslink = 1,
)

cc_library(
    name = "location_opencv",
    srcs = ["location_opencv.cc"],
    hdrs = ["location_opencv.h"],
    deps = [
        ":location",
        "//mediapipe/framework/formats/annotation:rasterization_cc_proto",
        "//mediapipe/framework/port:opencv_imgproc",
        "@com_google_absl//absl/log:absl_log",
    ],
    alwayslink = 1,
)

cc_test(
    name = "location_opencv_test",
    srcs = ["location_opencv_test.cc"],
    deps = [
        ":location_opencv",
        "//mediapipe/framework/formats/annotation:rasterization_cc_proto",
        "//mediapipe/framework/port:gtest_main",
        "//mediapipe/framework/port:rectangle",
    ],
)

cc_library(
    name = "video_stream_header",
    hdrs = ["video_stream_header.h"],
    deps = [
        ":image_format_cc_proto",
    ],
)

cc_library(
    name = "yuv_image",
    hdrs = ["yuv_image.h"],
    deps = [
        "//mediapipe/framework/port:integral_types",
        "@libyuv",
    ],
)

cc_test(
    name = "image_frame_opencv_test",
    size = "small",
    srcs = ["image_frame_opencv_test.cc"],
    deps = [
        ":image_format_cc_proto",
        ":image_frame",
        ":image_frame_opencv",
        "//mediapipe/framework/port:gtest_main",
        "//mediapipe/framework/port:integral_types",
        "//mediapipe/framework/port:logging",
    ],
)

mediapipe_proto_library(
    name = "rect_proto",
    srcs = ["rect.proto"],
)

mediapipe_register_type(
    base_name = "rect",
    include_headers = ["mediapipe/framework/formats/rect.pb.h"],
    types = [
        "::mediapipe::Rect",
        "::mediapipe::NormalizedRect",
        "::std::vector<::mediapipe::Rect>",
        "::std::vector<::mediapipe::NormalizedRect>",
    ],
    deps = [":rect_cc_proto"],
)

mediapipe_proto_library(
    name = "landmark_proto",
    srcs = ["landmark.proto"],
)

mediapipe_register_type(
    base_name = "landmark",
    include_headers = ["mediapipe/framework/formats/landmark.pb.h"],
    types = [
        "::mediapipe::Landmark",
        "::mediapipe::LandmarkList",
        "::mediapipe::LandmarkListCollection",
        "::mediapipe::NormalizedLandmark",
        "::mediapipe::NormalizedLandmarkList",
        "::mediapipe::NormalizedLandmarkListCollection",
        "::std::vector<::mediapipe::Landmark>",
        "::std::vector<::mediapipe::LandmarkList>",
        "::std::vector<::mediapipe::NormalizedLandmark>",
        "::std::vector<::mediapipe::NormalizedLandmarkList>",
    ],
    deps = [":landmark_cc_proto"],
)

cc_library(
    name = "image",
    srcs = ["image.cc"],
    hdrs = ["image.h"],
    copts = select({
        "//mediapipe:ios": [
            "-x objective-c++",
            "-fobjc-arc",  # enable reference-counting
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":image_format_cc_proto",
        ":image_frame",
        "//mediapipe/framework:port",
        "//mediapipe/framework:type_map",
        "//mediapipe/framework/port:logging",
        "//mediapipe/gpu:gpu_buffer",
        "//mediapipe/gpu:gpu_buffer_format",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/synchronization",
    ] + select({
        "//conditions:default": [
            "//mediapipe/gpu:gl_texture_buffer",
        ],
        "ios_or_disable_gpu": [],
    }) + select({
        "//conditions:default": [],
        "//mediapipe:apple": [
            "//mediapipe/objc:CFHolder",
            "//mediapipe/objc:util",
        ],
    }),
)

cc_library(
    name = "image_multi_pool",
    srcs = ["image_multi_pool.cc"],
    hdrs = ["image_multi_pool.h"],
    deps = [
        ":image",
        ":image_frame_pool",
        "//mediapipe/framework:port",
        "//mediapipe/framework/port:logging",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/synchronization",
    ] + select({
        "//conditions:default": [
            "//mediapipe/gpu:gl_base",
            "//mediapipe/gpu:gl_texture_buffer",
            "//mediapipe/gpu:gl_texture_buffer_pool",
            "//mediapipe/gpu:gpu_buffer",
        ],
        "//mediapipe:ios": [
            "//mediapipe/gpu:gl_base",
            "//mediapipe/gpu:gpu_buffer",
        ],
        "//mediapipe/gpu:disable_gpu": [],
    }) + select({
        "//conditions:default": [],
        "//mediapipe:apple": [
            "//mediapipe/gpu:pixel_buffer_pool_util",
            "//mediapipe/objc:CFHolder",
        ],
    }),
)

cc_library(
    name = "image_opencv",
    srcs = [
        "image_opencv.cc",
    ],
    hdrs = [
        "image_opencv.h",
    ],
    deps = [
        ":image",
        ":image_format_cc_proto",
        "//mediapipe/framework/port:logging",
        "//mediapipe/framework/port:opencv_core",
        "//mediapipe/framework/port:statusor",
        "@com_google_absl//absl/log:absl_check",
    ],
)

cc_library(
    name = "image_frame_pool",
    srcs = ["image_frame_pool.cc"],
    hdrs = ["image_frame_pool.h"],
    deps = [
        ":image_frame",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "image_frame_pool_test",
    size = "small",
    srcs = ["image_frame_pool_test.cc"],
    tags = ["linux"],
    deps = [
        ":image_frame_pool",
        "//mediapipe/framework/port:gtest_main",
        "//mediapipe/framework/port:status",
        "@com_google_absl//absl/memory",
    ],
)

# Used by vendor processes that don't have access to libandroid.so, but want to use AHardwareBuffer.
config_setting(
    name = "android_link_native_window",
    define_values = {
        "MEDIAPIPE_ANDROID_LINK_NATIVE_WINDOW": "1",
        "MEDIAPIPE_NO_JNI": "1",
    },
    values = {"crosstool_top": "//external:android/crosstool"},
    visibility = ["//visibility:private"],
)

cc_library(
    name = "tensor",
    srcs =
        [
            "tensor.cc",
            "tensor_ahwb.cc",
        ],
    hdrs = [
        "tensor.h",
        "//mediapipe/framework/formats/tensor:internal.h",
    ] + select({
        "//mediapipe:ios": ["tensor_mtl_buffer_view.h"],
        "//conditions:default": [],
    }),
    copts = select({
        "//mediapipe:apple": [
            "-x objective-c++",
            "-fobjc-arc",  # enable reference-counting
        ],
        "//conditions:default": [],
    }),
    defines = select({
        # Excludes AHardwareBuffer features from vendor processes
        "//mediapipe/framework:android_no_jni": ["MEDIAPIPE_NO_JNI"],
        # unless they're linked against nativewindow.
        ":android_link_native_window": [
            "MEDIAPIPE_ANDROID_LINK_NATIVE_WINDOW",
            "MEDIAPIPE_NO_JNI",
        ],
        "//conditions:default": [],
    }),
    linkopts = select({
        "//mediapipe:ios": [
            "-framework CoreVideo",
            "-framework MetalKit",
        ],
        "//conditions:default": [],
        "//mediapipe/framework:android_no_jni": [],
        "//mediapipe:android": [
            "-landroid",
        ],
        ":android_link_native_window": [
            "-lnativewindow",  # Provides <android/hardware_buffer.h> to vendor processes on Android API >= 26.
        ],
    }),
    deps = [
        "//mediapipe/framework:port",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/synchronization",
    ] + select({
        "//mediapipe/gpu:disable_gpu": [],
        "//conditions:default": [
            "//mediapipe/gpu:gl_base",
            "//mediapipe/gpu:gl_context",
        ],
    }),
)

cc_test(
    name = "tensor_test",
    srcs = ["tensor_test.cc"],
    tags = ["not_run:arm"],
    deps = [
        ":tensor",
        "//mediapipe/framework/port:gtest_main",
    ] + select({
        "//conditions:default": [
            "//mediapipe/gpu:gl_calculator_helper",
            "//mediapipe/gpu:gpu_buffer_format",
        ],
        "//mediapipe/gpu:disable_gpu": [],
    }),
)

cc_library(
    name = "frame_buffer",
    srcs = ["frame_buffer.cc"],
    hdrs = ["frame_buffer.h"],
    deps = [
        "//mediapipe/framework/port:integral_types",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)
