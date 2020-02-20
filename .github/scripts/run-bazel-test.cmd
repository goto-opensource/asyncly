bazel build //... || EXIT /B 1

bazel test --test_output=all //... || EXIT /B 1

bazel run -c opt //Test/Performance/... || EXIT /B 1
