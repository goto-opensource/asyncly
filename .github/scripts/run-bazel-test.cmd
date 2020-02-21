set "BAZEL_VS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise"

bazel test --test_output=all //... || EXIT /B 1

bazel run -c opt //Test/Performance/... || EXIT /B 1
