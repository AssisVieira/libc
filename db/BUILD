cc_library(
    name = "db",
    srcs = glob(["*.c", "*.h"]),
    hdrs = glob(["*.h"]),
    visibility = ["//visibility:public"],
    deps = ["//log", "//ioevent"],
    linkopts = ["-lpq"]
)


cc_binary(
    name = "test",
    srcs = glob(["test.c"]),
    linkstatic = True,
    deps = [":db"],
    data = ["logs"],
    visibility = ["//visibility:public"]
)
