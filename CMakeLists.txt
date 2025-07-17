cmake_minimum_required(VERSION 3.15)
project(RMNLib C)
set(CMAKE_C_STANDARD 11)

# ---- Find and include libcurl ----
find_package(CURL REQUIRED)

# All hand-written sources - collect from all subdirectories
file(GLOB ALL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c"
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

# Tests
enable_testing()
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c")
add_executable(RMNLib_runTests ${TEST_SOURCES})

# Tests executable include paths
target_include_directories(RMNLib_runTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
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
