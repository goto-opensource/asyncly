load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def asyncly_repositories():
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "e4fbb85eec69e6668ad397ec71a3a3ab165903abe98a8327db920b94508f720e",
        strip_prefix = "benchmark-1.5.3",
        urls = [
            "https://github.com/google/benchmark/archive/v1.5.3.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5",
        strip_prefix = "googletest-release-1.11.0",
        urls = [
            "https://github.com/google/googletest/archive/release-1.11.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = "e021e76e8e933672f1af0d223307282004f585a054354f8d894db39debddff8e",
        strip_prefix = "prometheus-cpp-0.12.3",
        urls = [
            "https://github.com/jupp0r/prometheus-cpp/archive/v0.12.3.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_naios_function2",
        sha256 = "c3aaeaf93bf90c0f4505a18f1094b51fe28881ce202c3bf78ec4efb336c51981",
        strip_prefix = "function2-4.1.0",
        urls = [
            "https://github.com/Naios/function2/archive/4.1.0.tar.gz",
        ],
        build_file = "@com_github_logmein_asyncly//Bazel:function2.BUILD",
    )

    maybe(
        http_archive,
        name = "com_github_nelhage_rules_boost",
        sha256 = "c5b128647a2492cf61374c030670469a16642c9cb3b134afcf59465231675388",
        strip_prefix = "rules_boost-2598b37ce68226fab465c0f0e10988af872b6dc9",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/2598b37ce68226fab465c0f0e10988af872b6dc9.tar.gz",
        ],
    )
