
asyncly_add_google_test(test_asyncly

  tests_main.cpp
  detail/PrometheusTestHelper.cpp
  detail/PrometheusTestHelper.h
  detail/ThrowingExecutor.h
  future/AddTimeoutTest.cpp
  future/CoroutineTest.cpp
  future/FutureTest.cpp
  future/LazyOneTimeInitializerTest.cpp
  future/SplitTest.cpp
  future/WhenAllTest.cpp
  future/WhenAnyTest.cpp
  observable/IObservableInterface.h
  observable/ObservableTest.cpp

  BaseSchedulerTest.cpp
  ExceptionShieldTest.cpp
  ExecutorCommonTest.cpp
  ExecutorThreadPoolTest.cpp
  InterfaceForExecutorTest.h
  MetricsWrapperTest.cpp
  StrandTest.cpp
  ThreadPoolExecutorTest.cpp
  WrapTest.cpp
)


if(NOT )
  set(COROUTINE_AWARE_SOURCES future/CoroutineTest.cpp observable/ObservableTest.cpp)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5 AND NOT RTC_OS_ANDROID)
    set_source_files_properties(${COROUTINE_AWARE_SOURCES} PROPERTIES COMPILE_FLAGS "-fcoroutines-ts")
  endif()

  if(MSVC_VERSION VERSION_GREATER_EQUAL 1910)
    set_source_files_properties(${COROUTINE_AWARE_SOURCES} PROPERTIES COMPILE_FLAGS "/AWAIT")
  endif()
endif()

target_include_directories(test_asyncly PRIVATE ${PROJECT_SOURCE_DIR}/Source)
target_include_directories(test_asyncly PRIVATE .)

target_link_libraries(test_asyncly asyncly asyncly_test_utils asyncly_mocks)
target_link_libraries(test_asyncly Boost::chrono Boost::thread Boost::system)
