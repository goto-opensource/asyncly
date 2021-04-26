
add_library(asyncly_mocks EXCLUDE_FROM_ALL

  # Enforce creation of a library for iOS, Mac
  dummy.cpp
  
  asyncly/MockIExecutor.h
  asyncly/MockIStrand.h
  asyncly/MockCancelable.h
)


target_include_directories(asyncly_mocks PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(asyncly_mocks PUBLIC asyncly)
target_link_libraries(asyncly_mocks PUBLIC GTest::gmock)
