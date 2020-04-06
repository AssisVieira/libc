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

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "actors_intern",
    srcs = [
        "actor.h",
        "meta_actor.h",
        "meta_msg.c",
        "meta_msg.h",
        "msg.c",
        "msg.h",
        "worker.c",
        "worker.h",
    ],
    hdrs = ["actors_intern.h"],
    defines = ["_GNU_SOURCE"],
    linkopts = ["-pthread"],
    visibility = ["//visibility:public"],
    deps = [
        "//log",
        "//queue",
        "//queue:queue_ss",
        "//vetor",
    ],
)

cc_test(
    name = "meta_msg_test",
    srcs = [
        "meta_msg_test.c",
    ],
    deps = [
        ":actors_intern",
        "//asserts",
    ],
)

cc_test(
    name = "msg_test",
    srcs = [
        "msg_test.c",
    ],
    deps = [
        ":actors_intern",
        "//asserts",
    ],
)

cc_test(
    name = "meta_actor_test",
    srcs = [
        "meta_actor_test.c",
    ],
    deps = [
        ":actors_intern",
        "//asserts",
    ],
)

cc_test(
    name = "worker_example",
    srcs = [
        "worker.c",
        "worker.h",
        "worker_example.c",
    ],
    linkopts = ["-pthread"],
    deps = [
        "//queue",
    ],
)
