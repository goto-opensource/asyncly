# ----------------------------------------------------------------------------------------
# Module setup
# ----------------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(asyncly CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CMake/AsynclyBuild.cmake)

option(BUILD_SHARED_LIBS "Build libraries as shared ones" OFF)
option(ENABLE_TESTING "Build tests" ON)
option(OVERRIDE_CXX_STANDARD_FLAGS "Force building with -std=c++14 even if the CXXLFAGS are configured differently" ON)

if(OVERRIDE_CXX_STANDARD_FLAGS)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(WIN32)
  add_definitions("-DWIN32_LEAN_AND_MEAN")
  add_definitions("-D_WIN32_WINNT=0x0601")
  add_compile_options("/bigobj")
endif()

find_package(function2 CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS chrono thread system)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

if(ENABLE_TESTING)
  find_package(benchmark CONFIG REQUIRED)
  find_package(GTest CONFIG REQUIRED)
  enable_testing()
endif()


# ----------------------------------------------------------------------------------------
# Targets
# ----------------------------------------------------------------------------------------

add_library(asyncly

  Source/executor/CurrentExecutor.cpp

  Source/executor/AsioExecutorController.cpp
  Source/executor/ExceptionShield.cpp
  Source/executor/InlineExecutor.cpp
  Source/executor/MetricsWrapper.cpp
  Source/executor/Strand.cpp
  Source/executor/ThreadPoolExecutorController.cpp

  Source/executor/detail/AsioExecutor.cpp
  Source/executor/detail/ExecutorMetrics.cpp
  Source/executor/detail/ExecutorMetrics.h
  Source/executor/detail/MetricsTask.cpp
  Source/executor/detail/MetricsTask.h
  Source/executor/detail/MetricsTaskState.cpp
  Source/executor/detail/MetricsTaskState.h
  Source/executor/detail/StrandImpl.cpp
  Source/executor/detail/StrandImpl.h
  Source/executor/detail/ThreadPoolExecutor.cpp

  Source/task/detail/PeriodicTask.cpp
  Source/task/detail/TaskCurrentExecutorGuard.cpp
)


if(RTC_OS_MSWIN)
  # silence deprecation warnings triggered by Boost Hana
  target_compile_definitions(asyncly PUBLIC _SILENCE_CXX17_IS_LITERAL_TYPE_DEPRECATION_WARNING _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING)
endif()

target_include_directories(asyncly PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>)
target_link_libraries(asyncly PUBLIC Boost::thread prometheus-cpp::core function2::function2)

add_subdirectory(Test)

# ----------------------------------------------------------------------------------------
# Installation
# ----------------------------------------------------------------------------------------

install(DIRECTORY Include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


install(TARGETS asyncly
  EXPORT ${PROJECT_NAME}-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

asyncly_install_export(
  EXPORT ${PROJECT_NAME}-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
)
