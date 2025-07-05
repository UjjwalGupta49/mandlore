#!/bin/bash
#
# This script configures and builds the C++ back-testing engine.
# It should be run from the root of the project directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

echo "--- CLEANING PREVIOUS BUILD ---"
rm -rf build

# --- Determine the number of available CPU cores for parallel compilation ---
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    CORES=$(nproc)
elif [[ "$OSTYPE" == "darwin"* ]]; then # macOS
    CORES=$(sysctl -n hw.ncpu)
else
    # Fallback for other systems (e.g., Windows with Git Bash)
    CORES=4
fi

echo "--- CONFIGURING CMAKE (using ${CORES} cores) ---"
# Create the build directory if it doesn't exist
mkdir -p build
# Navigate into the build directory
cd build
# Run CMake to generate the build files
cmake ..

echo ""
echo "--- COMPILING PROJECT ---"
# Run make to compile the project in parallel
make -j${CORES}

echo ""
echo "--- BUILD COMPLETE ---"
echo "The executable is located at: ./build/backtest_runner"
echo "You can now run a backtest, for example:"
echo "  ./build/backtest_runner Crypto.SOL/USD 60 1672531200 1735689600 buy_and_hold" 