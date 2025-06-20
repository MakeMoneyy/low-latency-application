cmake_minimum_required(VERSION 3.10)
project(LowLatencyTradingSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release build optimizations
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find Aeron
find_path(AERON_INCLUDE_DIR aeron/Aeron.h HINTS /usr/local/include)
find_library(AERON_CLIENT_LIB aeron_client HINTS /usr/local/lib)
find_library(AERON_DRIVER_LIB aeron_driver HINTS /usr/local/lib)

if(NOT AERON_INCLUDE_DIR OR NOT AERON_CLIENT_LIB)
    message(FATAL_ERROR "Aeron not found. Please install Aeron C++ client library.")
endif()

# Include directories
include_directories(${AERON_INCLUDE_DIR})
include_directories(include)
include_directories(third_party/spdlog/include)

# Source files
set(COMMON_SOURCES
    src/common/Config.cpp
    src/common/Logger.cpp
    src/common/TimeUtils.cpp
    src/common/DCIndicator.cpp
)

set(MARKET_DATA_SOURCES
    src/market_data/MarketDataProcessor.cpp
    src/market_data/DCEventDetector.cpp
)

set(STRATEGY_SOURCES
    src/strategy/StrategyEngine.cpp
    src/strategy/DCStrategy.cpp
)

set(EXECUTION_SOURCES
    src/execution/ExecutionEngine.cpp
    src/execution/OrderManager.cpp
)

# Libraries
add_library(Common ${COMMON_SOURCES})
add_library(MarketData ${MARKET_DATA_SOURCES})
add_library(Strategy ${STRATEGY_SOURCES})
add_library(Execution ${EXECUTION_SOURCES})

# Executables
add_executable(market_data_processor src/main/market_data_main.cpp)
add_executable(strategy_engine src/main/strategy_main.cpp)
add_executable(execution_engine src/main/execution_main.cpp)
add_executable(trading_system src/main/trading_system_main.cpp)

# Link libraries
target_link_libraries(Common ${AERON_CLIENT_LIB} Threads::Threads)
target_link_libraries(MarketData Common ${AERON_CLIENT_LIB})
target_link_libraries(Strategy Common ${AERON_CLIENT_LIB})
target_link_libraries(Execution Common ${AERON_CLIENT_LIB})

target_link_libraries(market_data_processor MarketData)
target_link_libraries(strategy_engine Strategy)
target_link_libraries(execution_engine Execution)
target_link_libraries(trading_system MarketData Strategy Execution)

# Copy config files to build directory
configure_file(config/system_config.json config/system_config.json COPYONLY)
configure_file(config/aeron_config.json config/aeron_config.json COPYONLY) 