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

load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "async",
    srcs = ["async.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "commit",
    srcs = ["commit.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "delete",
    srcs = ["delete.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "insert",
    srcs = ["insert.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "pool",
    srcs = ["pool.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "poolAsync",
    srcs = ["poolAsync.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "rollback",
    srcs = ["rollback.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "select",
    srcs = ["select.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)

cc_binary(
    name = "update",
    srcs = ["update.c"],
    linkstatic = True,
    deps = ["//db"],
    visibility = ["//visibility:public"]
)