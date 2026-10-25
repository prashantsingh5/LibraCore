cmake_minimum_required(VERSION 3.14)
project(BookManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bookmgmt
    src/Money.cpp
    src/Resource.cpp
    src/Book.cpp
    src/MT26076_Journal.cpp
    src/ElectronicResource.cpp
    src/MT26076_EBook.cpp
    src/MT26076_AudioBook.cpp
    src/MT26076_Thesis.cpp
    src/Budget.cpp
    src/Lending.cpp
    src/Catalog.cpp
    src/Acquisition.cpp
)
target_include_directories(bookmgmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(demo examples/demo.cpp)
target_link_libraries(demo PRIVATE bookmgmt)

enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/test_*.cpp)
add_executable(bookmgmt_tests ${TEST_SOURCES})
target_link_libraries(bookmgmt_tests PRIVATE bookmgmt)
add_test(NAME bookmgmt_tests COMMAND bookmgmt_tests)

add_executable(bookmgmt_benchmark benchmarks/benchmark_catalog.cpp)
target_link_libraries(bookmgmt_benchmark PRIVATE bookmgmt)

add_executable(bookmgmt_fault_tests tests/fault_injection.cpp)
target_link_libraries(bookmgmt_fault_tests PRIVATE bookmgmt)
add_test(NAME bookmgmt_fault_tests COMMAND bookmgmt_fault_tests)

foreach(target bookmgmt demo bookmgmt_tests bookmgmt_benchmark bookmgmt_fault_tests)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()
# Keep assert-based tests active even in Release builds.
if(MSVC)
    target_compile_options(bookmgmt_tests PRIVATE /UNDEBUG)
    target_compile_options(bookmgmt_fault_tests PRIVATE /UNDEBUG)
else()
    target_compile_options(bookmgmt_tests PRIVATE -UNDEBUG)
    target_compile_options(bookmgmt_fault_tests PRIVATE -UNDEBUG)
endif()

option(LIBRACORE_V2 "Build PostgreSQL application and Python bindings" OFF)
if(LIBRACORE_V2)
    set_target_properties(bookmgmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
    find_package(PostgreSQL REQUIRED)
    find_package(nlohmann_json 3.10 REQUIRED)
    find_path(PQXX_INCLUDE_DIR pqxx/pqxx REQUIRED)
    find_library(PQXX_LIBRARY NAMES pqxx REQUIRED)
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter Development.Module)
    set(PYBIND11_FINDPYTHON ON)
    find_package(pybind11 2.11 CONFIG REQUIRED)
    add_library(libracore_persistence src/v2/Mapping.cpp src/v2/PostgresRepository.cpp)
    set_target_properties(libracore_persistence PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_include_directories(libracore_persistence PUBLIC include ${PQXX_INCLUDE_DIR} ${PostgreSQL_INCLUDE_DIRS})
    target_link_libraries(libracore_persistence PUBLIC bookmgmt nlohmann_json::nlohmann_json ${PQXX_LIBRARY} PostgreSQL::PostgreSQL)
    target_compile_options(libracore_persistence PRIVATE -Wall -Wextra -Wpedantic)
    pybind11_add_module(_libracore bindings/module.cpp)
    target_link_libraries(_libracore PRIVATE libracore_persistence)
    target_compile_options(_libracore PRIVATE -Wall -Wextra -Wpedantic)
    set_target_properties(_libracore PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python")
    add_executable(libracore_db_tests tests/v2/test_postgres.cpp)
    target_link_libraries(libracore_db_tests PRIVATE libracore_persistence)
    target_compile_options(libracore_db_tests PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    add_executable(libracore_mapping_tests tests/v2/test_mapping.cpp)
    target_link_libraries(libracore_mapping_tests PRIVATE libracore_persistence)
    target_compile_options(libracore_mapping_tests PRIVATE -Wall -Wextra -Wpedantic -UNDEBUG)
    add_test(NAME libracore_mapping_tests COMMAND libracore_mapping_tests)
endif()
