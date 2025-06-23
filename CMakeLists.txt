cmake_minimum_required(VERSION 3.20)

project(backtest_engine LANGUAGES CXX)

# ----------------------------------------------------------------------------------
# Global compile options
# ----------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -march=native -pthread)
endif()

# ----------------------------------------------------------------------------------
# Header-only engine target (will grow as src files are added)
# ----------------------------------------------------------------------------------
add_library(backtest_engine INTERFACE)

target_include_directories(backtest_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------------
# GoogleTest (via FetchContent)
# ----------------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent GoogleTest from overriding our compiler/linker options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ----------------------------------------------------------------------------------
# Enable testing & add test subdirectory
# ----------------------------------------------------------------------------------
include(CTest)

enable_testing()
add_subdirectory(tests) 