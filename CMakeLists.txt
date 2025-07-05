cmake_minimum_required(VERSION 3.20)

project(backtest_engine LANGUAGES CXX)

# Find required system packages upfront
find_package(Threads REQUIRED)

# ----------------------------------------------------------------------------------
# Global compile options
# ----------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define our project's strict compile options in a variable
if (MSVC)
    set(MY_PROJECT_COMPILE_OPTIONS /W4 /WX)
else()
    set(MY_PROJECT_COMPILE_OPTIONS -Wall -Wextra -Wpedantic -Werror -march=native)
endif()

# ----------------------------------------------------------------------------------
# Header-only engine target (will grow as src files are added)
# ----------------------------------------------------------------------------------
add_library(backtest_engine INTERFACE)

target_include_directories(backtest_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ----------------------------------------------------------------------------------
# Add Source files to library (will be populated over time)
# ----------------------------------------------------------------------------------
target_sources(backtest_engine INTERFACE
    # --- Add .cpp files below ---
    src/data/CsvPriceSource.cpp
    src/data/PythPriceSource.cpp
    src/data/PriceManager.cpp
    src/data/Aggregator.cpp
    src/strategy/StrategyFactory.cpp
)

# ----------------------------------------------------------------------------------
# Third-party libraries (via FetchContent)
# ----------------------------------------------------------------------------------
include(FetchContent)

# --- nlohmann_json ---
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# --- cpr (HTTP client) ---
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/whoshuu/cpr.git
    GIT_TAG 1.10.5
)
FetchContent_MakeAvailable(cpr)


# Link dependencies to the main engine library
target_link_libraries(backtest_engine INTERFACE 
    nlohmann_json::nlohmann_json
    cpr::cpr
    Threads::Threads
)

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

# ----------------------------------------------------------------------------------
# Main Executable
# ----------------------------------------------------------------------------------
add_executable(backtest_runner main.cpp)
target_compile_options(backtest_runner PRIVATE ${MY_PROJECT_COMPILE_OPTIONS})
target_link_libraries(backtest_runner PRIVATE 
    backtest_engine
    buy_and_hold_strategy
    Threads::Threads # For std::thread
)

# ----------------------------------------------------------------------------------
# Strategies
# ----------------------------------------------------------------------------------
add_library(buy_and_hold_strategy strategies/buy_and_hold/BuyAndHoldStrategy.cpp)
target_include_directories(buy_and_hold_strategy PUBLIC strategies)
target_compile_options(buy_and_hold_strategy PRIVATE ${MY_PROJECT_COMPILE_OPTIONS})
target_link_libraries(buy_and_hold_strategy PRIVATE backtest_engine) 