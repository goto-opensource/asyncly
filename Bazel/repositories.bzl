load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def asyncly_repositories():
    maybe(
        http_archive,
        name = "com_github_google_benchmark",
        sha256 = "3e7059b6b11fb1bbe28e33e02519398ca94c1818874ebed18e504dc6f709be45",
        strip_prefix = "benchmark-1.8.4",
        urls = [
            "https://github.com/google/benchmark/archive/v1.8.4.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_google_googletest",
        sha256 = "8ad598c73ad796e0d8280b082cebd82a630d73e73cd3c70057938a6501bba5d7",
        strip_prefix = "googletest-1.14.0",
        urls = [
            "https://github.com/google/googletest/archive/v1.14.0.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_jupp0r_prometheus_cpp",
        sha256 = "48dbad454d314b836cc667ec4def93ec4a6e4255fc8387c20cacb3b8b6faee30",
        strip_prefix = "prometheus-cpp-1.2.4",
        urls = [
            "https://github.com/jupp0r/prometheus-cpp/archive/v1.2.4.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "com_github_naios_function2",
        sha256 = "6081d0f7011ddb8555bd846caf1245d4bce62d83fee1403b9d247b66ed617a67",
        strip_prefix = "function2-4.2.4",
        urls = [
            "https://github.com/Naios/function2/archive/4.2.4.tar.gz",
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
