# ----------------------------------------------------------------------------------------
# Module setup
# ----------------------------------------------------------------------------------------


cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(asyncly CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
include(CMake/AsynclyBuild.cmake)

option(BUILD_SHARED_LIBS "Build libraries as shared ones" OFF)
option(ENABLE_TESTING "Build tests" ON)

if(WIN32)
  add_definitions("-DNOMINMAX")
  add_definitions("-DWIN32_LEAN_AND_MEAN")
  add_definitions("-D_WIN32_WINNT=0x0601")
  add_compile_options("/bigobj")
  add_compile_options("/Zc:__cplusplus")
endif()

find_package(function2 CONFIG REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(Boost REQUIRED)

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
  Source/executor/ExternalEventExecutorController.cpp
  Source/executor/InlineExecutor.cpp
  Source/executor/MetricsWrapper.cpp
  Source/executor/Strand.cpp
  Source/executor/ThreadPoolExecutorController.cpp

  Source/executor/detail/AsioExecutor.cpp
  Source/executor/detail/ExecutorMetrics.cpp
  Source/executor/detail/ExternalEventExecutor.cpp
  Source/executor/detail/MetricsTask.cpp
  Source/executor/detail/MetricsTask.h
  Source/executor/detail/StrandImpl.cpp
  Source/executor/detail/StrandImpl.h

  Source/task/detail/PeriodicTask.cpp
  Source/task/detail/TaskCurrentExecutorGuard.cpp
)


target_compile_features(asyncly PUBLIC cxx_std_20)
target_include_directories(asyncly PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Include>)
target_link_libraries(asyncly PUBLIC Boost::boost prometheus-cpp::core function2::function2)

# EXTERNAL-ONLY if(ENABLE_TESTING)
  add_subdirectory(Test)
# EXTERNAL-ONLY endif()

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

