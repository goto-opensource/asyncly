asyncly_add_google_benchmark(asyncly_benchmarks
  EXCLUDE_FROM_TEST
  SOURCES
    Executor/ExecutorBenchmarksMain.cpp
    Executor/ExecutorWrappersPerformance.cpp
    Executor/ThreadPoolPerformanceTest.cpp)


target_include_directories(asyncly_benchmarks PRIVATE ${PROJECT_SOURCE_DIR}/Source)

target_link_libraries(asyncly_benchmarks asyncly)
