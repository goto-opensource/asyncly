add_library(asyncly_test_utils
  Include/asyncly/test/ConvertExecutorFutureToStdFuture.h
  Include/asyncly/test/ExecutorInterfaceTest.h
  Include/asyncly/test/ExecutorTestFactories.h
  Include/asyncly/test/FakeClockScheduler.h
  Include/asyncly/test/FakeExecutor.h
  Include/asyncly/test/FakeFutureTest.h
  Include/asyncly/test/FutureTest.h
  Include/asyncly/test/IFakeExecutor.h
  Include/asyncly/test/LazyValue.h
  Include/asyncly/test/PostAndWait.h
  Include/asyncly/test/SchedulerProvider.h
  Include/asyncly/test/WaitForFuture.h
  Include/asyncly/test/CurrentExecutorGuard.h
  Source/TimeoutGuard.cpp
  Source/TimeoutGuard.h
  Source/TimeoutGuardEnvironment.cpp
  Source/WindowsTimerResolution.cpp
  )

target_include_directories(asyncly_test_utils PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>)
target_link_libraries(asyncly_test_utils PUBLIC asyncly GTest::gmock)


install(DIRECTORY Include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS asyncly_test_utils
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
# BEGIN-INTERNAL
#]]
# END-INTERNAL