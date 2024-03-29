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
        sha256 = "ad7fdba11ea011c1d925b3289cf4af2c66a352e18d4c7264392fead75e919363",
        strip_prefix = "googletest-1.13.0",
        urls = [
            "https://github.com/google/googletest/archive/v1.13.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = "397544fe91e183029120b4eebcfab24ed9ec833d15850aae78fd5db19062d13a",
        strip_prefix = "prometheus-cpp-1.1.0",
        urls = [
            "https://github.com/jupp0r/prometheus-cpp/archive/v1.1.0.tar.gz",
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
        sha256 = "a7c42df432fae9db0587ff778d84f9dc46519d67a984eff8c79ae35e45f277c1",
        strip_prefix = "rules_boost-cfa585b1b5843993b70aa52707266dc23b3282d0",
        urls = [
            "https://github.com/nelhage/rules_boost/archive/cfa585b1b5843993b70aa52707266dc23b3282d0.tar.gz",
        ],
    )
