cmake_minimum_required(VERSION 3.21)

project(Fountain VERSION 0.1.0 LANGUAGES C CXX)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FOUNTAIN_BUILD_TESTS "Build Fountain unit tests" ON)
option(FOUNTAIN_BUILD_EXAMPLES "Build Fountain examples" ON)

include(FetchContent)

find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(SQLite3 REQUIRED)
if(NOT TARGET SQLite3::SQLite3 AND TARGET SQLite::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()

add_library(fountain STATIC
    src/fountain_c_api.cpp
    src/fountain_runtime.cpp
    src/event_json_nlohmann.cpp
    src/queue_repository.cpp
    src/time_utils.cpp
    src/id_utils.cpp
    src/backoff.cpp
    src/sqlite/db.cpp
    src/sqlite/statement.cpp
)

target_include_directories(fountain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(fountain
    PRIVATE
        nlohmann_json::nlohmann_json
        SQLite3::SQLite3
)

if(FOUNTAIN_BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    add_subdirectory(tests)
endif()

if(FOUNTAIN_BUILD_EXAMPLES)
    add_subdirectory(examples/cpp_basic)
endif()
