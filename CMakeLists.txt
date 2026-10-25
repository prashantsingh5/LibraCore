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
