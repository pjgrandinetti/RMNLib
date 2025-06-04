cmake_minimum_required(VERSION 3.15)
project(RMNLib C)
set(CMAKE_C_STANDARD 11)

# All hand-written sources
file(GLOB ALL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Build the static library
add_library(RMNLib STATIC ${ALL_SOURCES})

# Public include dirs: source, generated headers, and third-party includes
target_include_directories(RMNLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/OCTypes/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SITypes/include
)

# Tests
enable_testing()
file(GLOB TEST_SOURCES "tests/*.c")
add_executable(runTests ${TEST_SOURCES})
target_include_directories(runTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/OCTypes/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/SITypes/include
)
target_link_libraries(runTests PRIVATE
    RMNLib
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SITypes/lib/libSITypes.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OCTypes/lib/libOCTypes.a"
    m
)
add_test(NAME runTests COMMAND runTests)
