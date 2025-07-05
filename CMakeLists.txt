cmake_minimum_required(VERSION 3.15)
project(RMNLib C)
set(CMAKE_C_STANDARD 11)

# All hand-written sources
file(GLOB ALL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Build the static library
add_library(RMNLib STATIC ${ALL_SOURCES})
# Include RMNLib sources and OCTypes + SITypes headers and generated binaries
target_include_directories(RMNLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/OCTypes/src
    ${CMAKE_BINARY_DIR}/OCTypes
    ${CMAKE_SOURCE_DIR}/SITypes/src
    ${CMAKE_BINARY_DIR}/SITypes
)

# Tests
enable_testing()
file(GLOB TEST_SOURCES "tests/*.c")
add_executable(RMNLib_runTests ${TEST_SOURCES})
# Tests executable
target_include_directories(RMNLib_runTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/OCTypes/src
    ${CMAKE_BINARY_DIR}/OCTypes
    ${CMAKE_SOURCE_DIR}/SITypes/src
    ${CMAKE_BINARY_DIR}/SITypes
)
target_link_libraries(RMNLib_runTests PRIVATE
    RMNLib
    OCTypes
    SITypes
    m
)
add_test(NAME RMNLib_runTests COMMAND RMNLib_runTests)
