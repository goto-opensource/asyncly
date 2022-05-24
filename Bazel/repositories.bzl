load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def asyncly_repositories():
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "6132883bc8c9b0df5375b16ab520fac1a85dc9e4cf5be59480448ece74b278d4",
        strip_prefix = "benchmark-1.6.1",
        urls = [
            "https://github.com/google/benchmark/archive/v1.6.1.tar.gz",
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
        sha256 = "c1298755d1e5f458a45c410c56fb7a8d2e44586413ef6e2d48dd83cc2eaf6a98",
        strip_prefix = "rules_boost-789a047e61c0292c3b989514f5ca18a9945b0029",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/789a047e61c0292c3b989514f5ca18a9945b0029.tar.gz",
        ],
    )
