# src/EterBase/tests/CMakeLists.txt

set(TEST_TARGETS
    test_tea
    test_crc32
    test_lzo
    test_file
    test_stl
    test_timer
    test_poly
)

foreach(TEST_NAME ${TEST_TARGETS})
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    target_link_libraries(${TEST_NAME} PRIVATE EterBase)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()
