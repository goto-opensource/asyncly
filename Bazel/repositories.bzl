load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def asyncly_repositories():
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "dccbdab796baa1043f04982147e67bb6e118fe610da2c65f88912d73987e700c",
        strip_prefix = "benchmark-1.5.2",
        urls = [
            "https://github.com/google/benchmark/archive/v1.5.2.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        strip_prefix = "googletest-release-1.10.0",
        urls = [
            "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = "2102609457f812dbeaaafd55736461fd0538fc7e7568174b1cdec43399dbded4",
        strip_prefix = "prometheus-cpp-0.12.1",
        urls = [
            "https://github.com/jupp0r/prometheus-cpp/archive/v0.12.1.tar.gz",
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
        sha256 = "ec2b32e5dba50b80c58d5800234cd3e16ccd218f2f074ecddf27532b5e9a71be",
        strip_prefix = "rules_boost-c839cd7130a2aabe1d514805a47f3e7ffa2d9d9e",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/c839cd7130a2aabe1d514805a47f3e7ffa2d9d9e.tar.gz",
        ],
    )
