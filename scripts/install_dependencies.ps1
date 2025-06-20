# Low-Latency Trading System - Dependency Installation Script
# This script installs required dependencies for Windows development environment

Write-Host "=== Low-Latency Trading System - Dependency Installation ===" -ForegroundColor Green

# Check if vcpkg is available
$vcpkgPath = Get-Command vcpkg -ErrorAction SilentlyContinue
if (-not $vcpkgPath) {
    Write-Host "vcpkg not found. Installing vcpkg..." -ForegroundColor Yellow
    
    # Clone vcpkg
    if (-not (Test-Path "C:\vcpkg")) {
        git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
        Set-Location C:\vcpkg
        .\bootstrap-vcpkg.bat
        .\vcpkg integrate install
    }
    
    # Add vcpkg to PATH
    $env:PATH += ";C:\vcpkg"
    Write-Host "vcpkg installed successfully!" -ForegroundColor Green
}

Write-Host "Installing required packages..." -ForegroundColor Yellow

# Install spdlog (logging library)
Write-Host "Installing spdlog..." -ForegroundColor Cyan
vcpkg install spdlog:x64-windows

# Install nlohmann-json (JSON library)
Write-Host "Installing nlohmann-json..." -ForegroundColor Cyan
vcpkg install nlohmann-json:x64-windows

# Install fmt (formatting library - dependency for spdlog)
Write-Host "Installing fmt..." -ForegroundColor Cyan
vcpkg install fmt:x64-windows

# Note: Aeron installation is more complex and will be handled separately
Write-Host "Note: Aeron C++ client requires manual installation" -ForegroundColor Yellow
Write-Host "Please refer to: https://github.com/real-logic/aeron" -ForegroundColor Yellow

Write-Host "=== Dependency Installation Complete ===" -ForegroundColor Green
Write-Host "Installed packages:" -ForegroundColor White
Write-Host "  - spdlog (logging)" -ForegroundColor White
Write-Host "  - nlohmann-json (JSON parsing)" -ForegroundColor White
Write-Host "  - fmt (string formatting)" -ForegroundColor White

Write-Host "`nNext steps:" -ForegroundColor Yellow
Write-Host "1. Install Aeron C++ client manually" -ForegroundColor White
Write-Host "2. Update CMakeLists.txt with vcpkg toolchain" -ForegroundColor White
Write-Host "3. Build the project with cmake" -ForegroundColor White 