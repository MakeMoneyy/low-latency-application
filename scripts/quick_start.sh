#!/bin/bash

# Low Latency Trading System - Quick Start Script
# This script helps you get the trading system up and running quickly

set -e

echo "=== Low Latency Trading System Quick Start ==="
echo ""

# Check if we're in the correct directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: Please run this script from the project root directory"
    exit 1
fi

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check dependencies
echo "Checking dependencies..."

if ! command_exists cmake; then
    echo "Error: cmake is not installed"
    echo "Please install cmake: sudo apt-get install cmake"
    exit 1
fi

if ! command_exists g++; then
    echo "Error: g++ is not installed"
    echo "Please install g++: sudo apt-get install build-essential"
    exit 1
fi

echo "Dependencies check passed!"
echo ""

# Create necessary directories
echo "Creating directories..."
mkdir -p build
mkdir -p logs
mkdir -p data
mkdir -p tests/results

# Build the project
echo "Building the project..."
cd build

if [ "$1" = "debug" ]; then
    echo "Building debug version..."
    cmake -DCMAKE_BUILD_TYPE=Debug ..
else
    echo "Building release version..."
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build completed successfully!"
echo ""

# Copy configuration files
echo "Setting up configuration..."
cd ..
if [ ! -f "build/config/system_config.json" ]; then
    cp config/system_config.json build/config/ 2>/dev/null || echo "Config file already in place"
fi

# Check if Aeron media driver is running
echo "Checking Aeron media driver..."
if pgrep -f "aeron" > /dev/null; then
    echo "Aeron media driver is already running"
else
    echo "Note: You may need to start Aeron media driver separately"
    echo "Refer to Aeron documentation for setup instructions"
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "To run the trading system:"
echo "  cd build"
echo "  ./trading_system config/system_config.json"
echo ""
echo "To run individual components:"
echo "  ./market_data_processor"
echo "  ./strategy_engine"
echo "  ./execution_engine"
echo ""
echo "For help and configuration options, see README.md"
echo ""

# Optionally start the system
read -p "Do you want to start the trading system now? (y/n): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Starting trading system..."
    cd build
    ./trading_system config/system_config.json
fi 