#!/bin/bash

# Low Latency Trading System - Run Script
# This script helps run the complete trading system with proper sequencing

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
CONFIG_FILE="$PROJECT_ROOT/config/system_config.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging function
log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Function to check if a process is running
is_running() {
    pgrep -f "$1" > /dev/null 2>&1
}

# Function to kill processes
cleanup() {
    log "Cleaning up processes..."
    
    # Kill trading system
    if is_running "trading_system"; then
        pkill -f "trading_system" || true
        log "Trading system stopped"
    fi
    
    # Kill market data simulator
    if is_running "market_data_simulator"; then
        pkill -f "market_data_simulator" || true
        log "Market data simulator stopped"
    fi
    
    # Kill Aeron media driver if we started it
    if [ "$STARTED_AERON" = "true" ] && is_running "aeron"; then
        pkill -f "aeron" || true
        log "Aeron media driver stopped"
    fi
    
    success "Cleanup completed"
}

# Set trap for cleanup on exit
trap cleanup EXIT INT TERM

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    error "Build directory not found: $BUILD_DIR"
    error "Please run 'make cmake-build' or './scripts/quick_start.sh' first"
    exit 1
fi

# Check if executables exist
if [ ! -f "$BUILD_DIR/trading_system" ]; then
    error "trading_system executable not found"
    error "Please build the project first"
    exit 1
fi

if [ ! -f "$BUILD_DIR/market_data_simulator" ]; then
    error "market_data_simulator executable not found"
    error "Please build the project first"
    exit 1
fi

# Check if config file exists
if [ ! -f "$CONFIG_FILE" ]; then
    error "Configuration file not found: $CONFIG_FILE"
    exit 1
fi

cd "$BUILD_DIR"

# Parse command line arguments
MODE="demo"
MESSAGES_PER_SEC=1000
DURATION=60

while [[ $# -gt 0 ]]; do
    case $1 in
        --mode)
            MODE="$2"
            shift 2
            ;;
        --rate)
            MESSAGES_PER_SEC="$2"
            shift 2
            ;;
        --duration)
            DURATION="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --mode MODE         Run mode: demo, test, or manual (default: demo)"
            echo "  --rate RATE         Messages per second for simulator (default: 1000)"
            echo "  --duration DURATION Duration in seconds for demo mode (default: 60)"
            echo "  --help              Show this help message"
            echo ""
            echo "Modes:"
            echo "  demo    - Run complete system automatically for specified duration"
            echo "  test    - Run quick test with high message rate"
            echo "  manual  - Start components individually (manual control)"
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

log "Starting Low Latency Trading System in $MODE mode"

# Check if Aeron media driver is running
if ! is_running "aeron"; then
    warning "Aeron media driver not found, attempting to start..."
    # Try to start Aeron media driver (this might not work depending on installation)
    if command -v aeron-media-driver >/dev/null 2>&1; then
        aeron-media-driver &
        STARTED_AERON=true
        sleep 2
        log "Aeron media driver started"
    else
        warning "Aeron media driver not found in PATH"
        warning "Please ensure Aeron media driver is running separately"
    fi
fi

case $MODE in
    "demo")
        log "Running demo mode for $DURATION seconds at $MESSAGES_PER_SEC msg/sec"
        
        # Start market data simulator in background
        log "Starting market data simulator..."
        ./market_data_simulator "$CONFIG_FILE" $MESSAGES_PER_SEC &
        SIMULATOR_PID=$!
        sleep 2
        
        # Start trading system in background
        log "Starting trading system..."
        ./trading_system "$CONFIG_FILE" &
        TRADING_SYSTEM_PID=$!
        sleep 2
        
        success "All components started successfully"
        log "System will run for $DURATION seconds..."
        
        # Monitor for specified duration
        for ((i=1; i<=DURATION; i++)); do
            if ! kill -0 $SIMULATOR_PID 2>/dev/null || ! kill -0 $TRADING_SYSTEM_PID 2>/dev/null; then
                error "One or more components stopped unexpectedly"
                break
            fi
            
            if ((i % 10 == 0)); then
                log "Running... ${i}/${DURATION} seconds"
            fi
            
            sleep 1
        done
        
        log "Demo completed"
        ;;
        
    "test")
        log "Running test mode with high message rate"
        MESSAGES_PER_SEC=10000
        DURATION=30
        
        # Start components
        log "Starting market data simulator at $MESSAGES_PER_SEC msg/sec..."
        ./market_data_simulator "$CONFIG_FILE" $MESSAGES_PER_SEC &
        sleep 2
        
        log "Starting trading system..."
        ./trading_system "$CONFIG_FILE" &
        sleep 2
        
        success "Test running for $DURATION seconds..."
        sleep $DURATION
        
        log "Test completed"
        ;;
        
    "manual")
        log "Manual mode - start components individually"
        log ""
        log "To start market data simulator:"
        log "  ./market_data_simulator $CONFIG_FILE [messages_per_second]"
        log ""
        log "To start trading system:"
        log "  ./trading_system $CONFIG_FILE"
        log ""
        log "Press Ctrl+C when done"
        
        # Wait for user to interrupt
        while true; do
            sleep 1
        done
        ;;
        
    *)
        error "Unknown mode: $MODE"
        exit 1
        ;;
esac

success "Trading system run completed" 