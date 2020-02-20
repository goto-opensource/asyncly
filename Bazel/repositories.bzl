load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def asyncly_repositories():
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "f8e525db3c42efc9c7f3bc5176a8fa893a9a9920bbd08cef30fb56a51854d60d",
        strip_prefix = "benchmark-1.4.1",
        urls = [
            "https://github.com/google/benchmark/archive/v1.4.1.tar.gz",
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
        sha256 = "85ad6fea0f0dcb413104366b7d6109acdb015aff8767945511c5cad8202a28a6",
        strip_prefix = "prometheus-cpp-0.9.0",
        urls = [
            "https://github.com/jupp0r/prometheus-cpp/archive/v0.9.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_naios_function2",
        sha256 = "6bc62a300cc54586f5ba0a2a04118ddf88c1076bd0329b21d7bed78f838baa43",
        strip_prefix = "function2-4.0.0",
        urls = [
            "https://github.com/Naios/function2/archive/4.0.0.tar.gz",
        ],
        build_file = "@com_github_logmein_asyncly//Bazel:function2.BUILD",
    )

    maybe(
        http_archive,
        name = "com_github_nelhage_rules_boost",
        sha256 = "1ea1cefb01d7dda89557a0c0f275ce684c7fc88bf19e91d35ec694cbd83c60e5",
        strip_prefix = "rules_boost-c425c956ffc94edaf4e0314cdd002dc0bdf23142",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/c425c956ffc94edaf4e0314cdd002dc0bdf23142.tar.gz",
        ],
    )
