################################################################################
#   Copyright 2020 Assis Vieira
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
################################################################################

load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "log",
    srcs = [
        "log.c",
        "log.h",
    ],
    hdrs = ["log.h"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "example1",
    srcs = ["example1.c"],
    visibility = ["//visibility:public"],
    deps = [":log"],
)

cc_binary(
    name = "example2",
    srcs = ["example2.c"],
    visibility = ["//visibility:public"],
    deps = [":log"],
)

cc_binary(
    name = "example3",
    srcs = ["example3.c"],
    visibility = ["//visibility:public"],
    deps = [":log"],
)
