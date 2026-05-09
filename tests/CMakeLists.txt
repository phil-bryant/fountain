add_executable(fountain_tests
    fountain_privacy_tests.cpp
    fountain_json_tests.cpp
    fountain_sqlite_tests.cpp
    fountain_batch_tests.cpp
    fountain_c_api_safety_tests.cpp
)

target_include_directories(fountain_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(fountain_tests
    PRIVATE
        fountain
        GTest::gtest
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        SQLite3::SQLite3
)

include(GoogleTest)
gtest_discover_tests(fountain_tests)
