cc_library(
    name = "hashTable",
    srcs = ["hashTable.c", "hashTable.h"],
    hdrs = ["hashTable.h"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "test",
    srcs = ["test.c"],
    deps = [":hashTable"],
)
