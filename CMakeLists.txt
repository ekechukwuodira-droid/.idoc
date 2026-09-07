cmake_minimum_required(VERSION 3.20)
project(idoc_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

# --- zstd (compression, §1.5) -------------------------------------------
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG v1.5.6
    SOURCE_SUBDIR build/cmake
)
FetchContent_MakeAvailable(zstd)

# --- nlohmann_json (Manifest, §1.3) -------------------------------------
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# --- doctest (tests) -----------------------------------------------------
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

# --- idoc_core: container + model + serde, Qt-free ----------------------
add_library(idoc_core STATIC
    src/container/file_header.cpp
    src/container/crc64.cpp
    src/container/tlv.cpp
    src/container/compression.cpp
    src/container/manifest.cpp
    src/container/block_directory.cpp
    src/container/container_writer.cpp
    src/container/container_reader.cpp
    src/serde/metadata_serde.cpp
    src/serde/common_serde.cpp
    src/serde/theme_serde.cpp
    src/serde/styles_serde.cpp
    src/serde/sections_serde.cpp
    src/serde/paragraph_serde.cpp
    src/serde/numbering_serde.cpp
    src/serde/fields_serde.cpp
    src/serde/tables_serde.cpp
    src/serde/annotations_serde.cpp
    src/serde/resources_serde.cpp
)
target_include_directories(idoc_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(idoc_core PUBLIC
    libzstd_static
    nlohmann_json::nlohmann_json
)

# --- idoc_cli: the no-UI test harness ------------------------------------
add_executable(idoc_cli cli/main.cpp)
target_link_libraries(idoc_cli PRIVATE idoc_core)

# --- idoc_tests: doctest suite --------------------------------------------
enable_testing()
add_executable(idoc_tests
    tests/test_main.cpp
    tests/test_crc64.cpp
    tests/test_tlv.cpp
    tests/test_file_header.cpp
    tests/test_manifest.cpp
    tests/test_metadata_serde.cpp
    tests/test_container_roundtrip.cpp
    tests/test_theme_serde.cpp
    tests/test_styles_serde.cpp
    tests/test_sections_serde.cpp
    tests/test_paragraph_serde.cpp
    tests/test_numbering_serde.cpp
    tests/test_fields_serde.cpp
    tests/test_tables_serde.cpp
    tests/test_annotations_serde.cpp
    tests/test_resources_serde.cpp
)
target_link_libraries(idoc_tests PRIVATE idoc_core doctest::doctest)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake OPTIONAL RESULT_VARIABLE DOCTEST_CMAKE_FOUND)
if(DOCTEST_CMAKE_FOUND)
    doctest_discover_tests(idoc_tests)
else()
    add_test(NAME idoc_tests COMMAND idoc_tests)
endif()
