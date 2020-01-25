cc_library(
    name = "vetor",
    srcs = glob(["vetor.c", "vetor.h"]),
    hdrs = ["vetor.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "teste",
    srcs = glob(["teste.c"]),
    deps = [":vetor"],
)
