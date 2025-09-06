cmake_minimum_required(VERSION 3.15)
project(RMNLib C)
set(CMAKE_C_STANDARD 11)

# ---- Find and include libcurl ----
find_package(CURL REQUIRED)

# ---- Find OpenMP (optional for parallel processing) ----
find_package(OpenMP)
if(OpenMP_C_FOUND)
    message(STATUS "OpenMP found - enabling parallel processing")
else()
    message(STATUS "OpenMP not found - using sequential processing")
endif()

# All hand-written sources - collect from all subdirectories
file(GLOB ALL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/dataset/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/dependent_variable/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/dimension/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/importers/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/spectroscopy/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.c"
)

# Build the static library
add_library(RMNLib STATIC ${ALL_SOURCES})

# Include RMNLib sources, OCTypes + SITypes headers, and generated binaries
target_include_directories(RMNLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/dataset
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/dependent_variable
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/dimension
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spectroscopy
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/OCTypes/src
    ${CMAKE_BINARY_DIR}/OCTypes
    ${CMAKE_SOURCE_DIR}/SITypes/src
    ${CMAKE_BINARY_DIR}/SITypes
    ${CURL_INCLUDE_DIRS}                # ← curl headers
)

# Link libcurl into your library
if(APPLE)
    target_link_libraries(RMNLib PUBLIC
        CURL::libcurl
        "-framework Accelerate"
    )
else()
    target_link_libraries(RMNLib PUBLIC
        CURL::libcurl
    )
endif()

# Link OpenMP if available
if(OpenMP_C_FOUND)
    target_link_libraries(RMNLib PUBLIC OpenMP::OpenMP_C)
endif()

# Tests
enable_testing()
# Collect test files but exclude the main files we don't want
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.c")
# Collect test header files
file(GLOB TEST_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.h")
# Add only main_all.c as the main file (it runs all tests)
list(APPEND TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/main_all.c")
add_executable(RMNLib_runTests ${TEST_SOURCES})

# Tests executable include paths
target_include_directories(RMNLib_runTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/dependent_variable
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/dimension
    ${CMAKE_CURRENT_SOURCE_DIR}/src/importers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/spectroscopy
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party
    ${CMAKE_SOURCE_DIR}/OCTypes/src
    ${CMAKE_BINARY_DIR}/OCTypes
    ${CMAKE_SOURCE_DIR}/SITypes/src
    ${CMAKE_BINARY_DIR}/SITypes
    ${CURL_INCLUDE_DIRS}                # ← curl headers for tests
)

# Link the test binary against RMNLib, OCTypes, SITypes and curl
if(APPLE)
    target_link_libraries(RMNLib_runTests PRIVATE
        RMNLib
        OCTypes
        SITypes
        m
        CURL::libcurl
        "-framework Accelerate"
    )
else()
    target_link_libraries(RMNLib_runTests PRIVATE
        RMNLib
        OCTypes
        SITypes
        m
        CURL::libcurl
    )
endif()

add_test(NAME RMNLib_runTests COMMAND RMNLib_runTests)
