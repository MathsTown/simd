cmake_minimum_required(VERSION 3.20)

project(simd_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif ()

set(SIMD_TEST_MODE "AVX2" CACHE STRING "SIMD test mode: SSE2, AVX2 or AVX512")
set_property(CACHE SIMD_TEST_MODE PROPERTY STRINGS SSE2 AVX2 AVX512)
option(SIMD_TEST_USE_SLEEF "Auto-download and build SLEEF inline headers for tests" OFF)
set(SIMD_TEST_SLEEF_GIT_TAG "master" CACHE STRING "SLEEF git tag/branch used when SIMD_TEST_USE_SLEEF=ON")

if (NOT SIMD_TEST_MODE STREQUAL "SSE2" AND NOT SIMD_TEST_MODE STREQUAL "AVX2" AND NOT SIMD_TEST_MODE STREQUAL "AVX512")
    message(FATAL_ERROR "SIMD_TEST_MODE must be SSE2, AVX2 or AVX512")
endif ()

add_executable(simd_test
    main.cpp
    test_f32.cpp
    test_f64.cpp
    test_i32.cpp
    test_i64.cpp
)

target_include_directories(simd_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../Include)

if (SIMD_TEST_MODE STREQUAL "SSE2")
    target_compile_definitions(simd_test PRIVATE SIMD_TEST_MIN_LEVEL=1)
else ()
    target_compile_definitions(simd_test PRIVATE SIMD_TEST_MIN_LEVEL=3)
endif ()

if (SIMD_TEST_USE_SLEEF)
    if (MSVC AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "SIMD_TEST_USE_SLEEF is supported for GCC/Clang test builds.")
    endif ()
    if (SIMD_TEST_MODE STREQUAL "SSE2")
        message(FATAL_ERROR "SIMD_TEST_USE_SLEEF requires SIMD_TEST_MODE=AVX2 or AVX512.")
    endif ()

    include(FetchContent)

    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install path for test dependencies" FORCE)
    set(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT FALSE)

    if (NOT DEFINED SED_COMMAND OR NOT EXISTS "${SED_COMMAND}")
        find_program(SIMD_TEST_SED_COMMAND
            NAMES sed
            HINTS
                "$ENV{ProgramFiles}/Git/usr/bin"
                "$ENV{ProgramW6432}/Git/usr/bin"
                "C:/Program Files/Git/usr/bin"
        )
        if (SIMD_TEST_SED_COMMAND)
            set(SED_COMMAND "${SIMD_TEST_SED_COMMAND}" CACHE FILEPATH "sed executable for SLEEF inline header generation" FORCE)
        endif ()
    endif ()

    if (NOT DEFINED SED_COMMAND OR NOT EXISTS "${SED_COMMAND}")
        message(FATAL_ERROR "SIMD_TEST_USE_SLEEF requires sed to generate SLEEF inline headers. Install Git for Windows or provide -DSED_COMMAND=<path-to-sed>.")
    endif ()

    # Keep SLEEF build minimal for test harness use.
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(SLEEF_BUILD_INLINE_HEADERS ON CACHE BOOL "" FORCE)
    set(SLEEF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(SLEEF_BUILD_DFT OFF CACHE BOOL "" FORCE)
    set(SLEEF_BUILD_GNUABI_LIBS OFF CACHE BOOL "" FORCE)
    set(SLEEF_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SLEEF_ENABLE_LTO OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        sleef
        GIT_REPOSITORY https://github.com/shibatch/sleef.git
        GIT_TAG ${SIMD_TEST_SLEEF_GIT_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sleef)

    target_compile_definitions(simd_test PRIVATE MT_USE_SLEEF=1)
    target_include_directories(simd_test SYSTEM PRIVATE ${sleef_BINARY_DIR}/include)

    # The inline headers are generated into the SLEEF binary include directory.
    if (TARGET inline_headers_util)
        add_dependencies(simd_test inline_headers_util)
    elseif (TARGET headers)
        add_dependencies(simd_test headers)
    endif ()
endif ()

if (MSVC OR CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    target_compile_options(simd_test PRIVATE /W4 /WX /EHsc /arch:AVX2)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(simd_test PRIVATE -Wall -Wextra -Werror)
    if (WIN32)
        target_compile_options(simd_test PRIVATE -mstackrealign)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT SIMD_TEST_MODE STREQUAL "SSE2")
            target_compile_options(simd_test PRIVATE -mavx256-split-unaligned-load -mavx256-split-unaligned-store)
        endif ()
    endif ()
    if (SIMD_TEST_MODE STREQUAL "AVX512")
        target_compile_options(simd_test PRIVATE -mavx2 -mfma -mavx512f -mavx512dq -mavx512vl -mavx512bw -mavx512cd)
    elseif (SIMD_TEST_MODE STREQUAL "AVX2")
        target_compile_options(simd_test PRIVATE -mavx2 -mfma)
    else ()
        target_compile_options(simd_test PRIVATE -msse2 -mno-avx -mno-avx2 -mno-fma)
    endif ()
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(simd_test PRIVATE -Wno-psabi)
    endif ()
endif ()

if (SIMD_TEST_USE_SLEEF)
    if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(simd_test PRIVATE /clang:-ffp-contract=off)
    else ()
        target_compile_options(simd_test PRIVATE -ffp-contract=off)
    endif ()
endif ()
