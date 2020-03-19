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
    name = "queue_ss",
    srcs = [
        "queue_ss.c",
        "queue_ss.h",
    ],
    hdrs = ["queue_ss.h"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"]
)

cc_test(
    name = "queue_ss_test",
    srcs = ["queue_ss_test.c"],
    visibility = ["//visibility:public"],
    deps = [":queue_ss"]
)

