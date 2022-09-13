load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def asyncly_repositories():
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "3aff99169fa8bdee356eaa1f691e835a6e57b1efeadb8a0f9f228531158246ac",
        strip_prefix = "benchmark-1.7.0",
        urls = [
            "https://github.com/google/benchmark/archive/v1.7.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "81964fe578e9bd7c94dfdb09c8e4d6e6759e19967e397dbea48d1c10e45d0df2",
        strip_prefix = "googletest-release-1.12.1",
        urls = [
            "https://github.com/google/googletest/archive/release-1.12.1.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = "593e028d401d3298eada804d252bc38d8cab3ea1c9e88bcd72095281f85e6d16",
        strip_prefix = "prometheus-cpp-1.0.1",
        urls = [
            "https://github.com/jupp0r/prometheus-cpp/archive/v1.0.1.tar.gz",
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
        sha256 = "0db14c7eb7acc78cbe601a450d7161e492d7ffc98150775cfef6e2cdf5ec7e6a",
        strip_prefix = "rules_boost-ea2991b24d68439121130845f7dbef8d1dd383d2",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/ea2991b24d68439121130845f7dbef8d1dd383d2.tar.gz",
        ],
    )
