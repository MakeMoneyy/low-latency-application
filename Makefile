# Low Latency Trading System Makefile
# Simple Makefile for quick development builds

CXX = g++
CXXFLAGS = -std=c++17 -O3 -DNDEBUG -march=native -Wall -Wextra
INCLUDES = -Iinclude -I/usr/local/include
LIBS = -L/usr/local/lib -laeron_client -lpthread -lspdlog

# Debug build flags
DEBUG_FLAGS = -std=c++17 -O0 -g -DDEBUG -Wall -Wextra

# Source directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
COMMON_SOURCES = $(SRC_DIR)/common/DCIndicator.cpp $(SRC_DIR)/common/TimeUtils.cpp $(SRC_DIR)/common/Config.cpp $(SRC_DIR)/common/Logger.cpp
MARKET_DATA_SOURCES = $(SRC_DIR)/market_data/MarketDataProcessor.cpp
STRATEGY_SOURCES = $(SRC_DIR)/strategy/StrategyEngine.cpp
EXECUTION_SOURCES = $(SRC_DIR)/execution/ExecutionEngine.cpp
MAIN_SOURCE = $(SRC_DIR)/main/trading_system_main.cpp
SIMULATOR_SOURCE = $(SRC_DIR)/main/market_data_simulator.cpp

# Default target
all: release

# Release build
release:
	@echo "Building release version..."
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(COMMON_SOURCES) $(MARKET_DATA_SOURCES) $(STRATEGY_SOURCES) $(EXECUTION_SOURCES) $(MAIN_SOURCE) $(LIBS) -o $(BUILD_DIR)/trading_system
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(COMMON_SOURCES) $(MARKET_DATA_SOURCES) $(SIMULATOR_SOURCE) $(LIBS) -o $(BUILD_DIR)/market_data_simulator

# Debug build
debug:
	@echo "Building debug version..."
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(DEBUG_FLAGS) $(INCLUDES) $(COMMON_SOURCES) $(MARKET_DATA_SOURCES) $(STRATEGY_SOURCES) $(EXECUTION_SOURCES) $(MAIN_SOURCE) $(LIBS) -o $(BUILD_DIR)/trading_system_debug
	$(CXX) $(DEBUG_FLAGS) $(INCLUDES) $(COMMON_SOURCES) $(MARKET_DATA_SOURCES) $(SIMULATOR_SOURCE) $(LIBS) -o $(BUILD_DIR)/market_data_simulator_debug

# Clean build files
clean:
	@echo "Cleaning build files..."
	@rm -rf $(BUILD_DIR)
	@rm -f *.log
	@rm -f *.json

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y build-essential cmake git
	sudo apt-get install -y libspdlog-dev nlohmann-json3-dev

# Create necessary directories
setup:
	@echo "Setting up project directories..."
	@mkdir -p $(BUILD_DIR)
	@mkdir -p logs
	@mkdir -p data
	@mkdir -p tests

# Run the system
run: release
	@echo "Starting trading system..."
	cd $(BUILD_DIR) && ./trading_system ../config/system_config.json

# CMake build (preferred for production)
cmake-build:
	@echo "Building with CMake..."
	@mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc)

# CMake debug build
cmake-debug:
	@echo "Building debug with CMake..."
	@mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make -j$(nproc)

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build release version (default)"
	@echo "  release      - Build optimized release version"
	@echo "  debug        - Build debug version"
	@echo "  clean        - Clean build files"
	@echo "  install-deps - Install system dependencies"
	@echo "  setup        - Create project directories"
	@echo "  run          - Build and run the system"
	@echo "  cmake-build  - Build using CMake (recommended)"
	@echo "  cmake-debug  - Build debug using CMake"
	@echo "  help         - Show this help message"

.PHONY: all release debug clean install-deps setup run cmake-build cmake-debug help