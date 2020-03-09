asyncly_add_self_contained_header_test(test_asyncly_self_contained "../../Include")
target_link_libraries(test_asyncly_self_contained PRIVATE asyncly GTest::gmock)

