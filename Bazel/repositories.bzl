load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def asyncly_repositories():
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "1f71c72ce08d2c1310011ea6436b31e39ccab8c2db94186d26657d41747c85d6",
        strip_prefix = "benchmark-1.6.0",
        urls = [
            "https://github.com/google/benchmark/archive/v1.6.0.tar.gz",
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
        sha256 = "07018db604ea3e61f5078583e87c80932ea10c300d979061490ee1b7dc8e3a41",
        strip_prefix = "prometheus-cpp-1.0.0",
        urls = [
            "https://github.com/jupp0r/prometheus-cpp/archive/v1.0.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_naios_function2",
        sha256 = "fd1194b236e55f695c3a0c17f2440d6965b800c9309d0d5937e0185bcfe7ae6e",
        strip_prefix = "function2-4.2.0",
        urls = [
            "https://github.com/Naios/function2/archive/4.2.0.tar.gz",
        ],
        build_file = "@com_github_logmein_asyncly//Bazel:function2.BUILD",
    )

    maybe(
        http_archive,
        name = "com_github_nelhage_rules_boost",
        sha256 = "c3264642c6f77a894c19432fed9b0c0d1ad156b56f8e32c13abac4c682bd0873",
        strip_prefix = "rules_boost-c8b9b4a75c4301778d2e256b8d72ce47a6c9a1a4",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/c8b9b4a75c4301778d2e256b8d72ce47a6c9a1a4.tar.gz",
        ],
    )
