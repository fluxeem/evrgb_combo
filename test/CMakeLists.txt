cmake_minimum_required(VERSION 3.22)

# Find Google Test
find_package(GTest REQUIRED)

# Add test executable
add_executable(test_combo_enumeration
    test_camera_enumeration.cpp
    test_dvs_enumeration.cpp
    test_combo_enumeration.cpp
)

# Link against the evrgb_combo library and Google Test
target_link_libraries(test_combo_enumeration
    evrgb_combo
    GTest::gtest
    GTest::gtest_main
)

# Include directories
target_include_directories(test_combo_enumeration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Enable testing
enable_testing()

# Add the test (simple version that always works)
add_test(NAME ComboEnumerationTest COMMAND test_combo_enumeration)

# Try to use gtest_discover_tests for better integration
# include(GoogleTest)
# if(COMMAND gtest_discover_tests)
#     gtest_discover_tests(test_combo_enumeration)
# endif()
