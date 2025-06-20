# Low-Latency Trading System - Production Environment Setup Script
# This script prepares the system for production deployment

param(
    [switch]$InstallDependencies,
    [switch]$SetupAeron,
    [switch]$ConfigureEnvironment,
    [switch]$All
)

Write-Host "=== Low-Latency Trading System - Production Setup ===" -ForegroundColor Green

if ($All) {
    $InstallDependencies = $true
    $SetupAeron = $true
    $ConfigureEnvironment = $true
}

# Function to check if running as administrator
function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Administrator)) {
    Write-Warning "This script should be run as Administrator for best results."
    Write-Host "Some operations may fail without admin privileges." -ForegroundColor Yellow
}

# Install dependencies using vcpkg
if ($InstallDependencies) {
    Write-Host "`n=== Installing Production Dependencies ===" -ForegroundColor Cyan
    
    # Check vcpkg installation
    $vcpkgPath = "C:\vcpkg\vcpkg.exe"
    if (-not (Test-Path $vcpkgPath)) {
        Write-Host "Installing vcpkg..." -ForegroundColor Yellow
        
        if (-not (Test-Path "C:\vcpkg")) {
            git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
        }
        
        Set-Location C:\vcpkg
        .\bootstrap-vcpkg.bat
        .\vcpkg integrate install
        
        # Add to system PATH
        $env:PATH += ";C:\vcpkg"
        [Environment]::SetEnvironmentVariable("PATH", $env:PATH, [EnvironmentVariableTarget]::Machine)
    }
    
    Write-Host "Installing required packages..." -ForegroundColor Yellow
    
    # Install production dependencies
    & $vcpkgPath install spdlog:x64-windows
    & $vcpkgPath install nlohmann-json:x64-windows
    & $vcpkgPath install fmt:x64-windows
    & $vcpkgPath install gtest:x64-windows
    & $vcpkgPath install benchmark:x64-windows
    
    Write-Host "Dependencies installed successfully!" -ForegroundColor Green
}

# Setup Aeron
if ($SetupAeron) {
    Write-Host "`n=== Setting up Aeron C++ Client ===" -ForegroundColor Cyan
    
    $aeronDir = "C:\aeron"
    
    if (-not (Test-Path $aeronDir)) {
        Write-Host "Downloading Aeron..." -ForegroundColor Yellow
        
        # Create directory
        New-Item -ItemType Directory -Path $aeronDir -Force
        
        # Download Aeron (using GitHub releases)
        $aeronVersion = "1.44.1"
        $downloadUrl = "https://github.com/real-logic/aeron/archive/refs/tags/$aeronVersion.zip"
        $zipPath = "$aeronDir\aeron-$aeronVersion.zip"
        
        try {
            Invoke-WebRequest -Uri $downloadUrl -OutFile $zipPath
            Expand-Archive -Path $zipPath -DestinationPath $aeronDir
            
            Write-Host "Aeron downloaded successfully!" -ForegroundColor Green
            Write-Host "Manual build required - see instructions in README" -ForegroundColor Yellow
        }
        catch {
            Write-Error "Failed to download Aeron: $_"
            Write-Host "Please download manually from: https://github.com/real-logic/aeron" -ForegroundColor Yellow
        }
    } else {
        Write-Host "Aeron directory already exists at $aeronDir" -ForegroundColor Yellow
    }
    
    # Create Aeron build instructions
    $buildInstructions = @"
# Aeron C++ Build Instructions

## Prerequisites
1. Install CMake (3.10 or later)
2. Install Visual Studio 2019 or later with C++ support
3. Install Java 8+ (required for Aeron build)

## Build Steps
1. Open PowerShell as Administrator
2. Navigate to Aeron directory:
   cd C:\aeron\aeron-$aeronVersion

3. Create build directory:
   mkdir build
   cd build

4. Configure with CMake:
   cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release

5. Build:
   cmake --build . --config Release

6. Install (optional):
   cmake --install . --prefix C:\aeron\install

## Environment Variables
Add to system PATH:
- C:\aeron\install\bin
- C:\aeron\install\lib

Add environment variable:
- AERON_DIR=C:\aeron\install
"@
    
    $buildInstructions | Out-File -FilePath "$aeronDir\BUILD_INSTRUCTIONS.txt" -Encoding UTF8
    Write-Host "Build instructions saved to $aeronDir\BUILD_INSTRUCTIONS.txt" -ForegroundColor Green
}

# Configure production environment
if ($ConfigureEnvironment) {
    Write-Host "`n=== Configuring Production Environment ===" -ForegroundColor Cyan
    
    # Create production directories
    $prodDirs = @(
        "C:\LowLatencyTrading\bin",
        "C:\LowLatencyTrading\config",
        "C:\LowLatencyTrading\logs",
        "C:\LowLatencyTrading\data",
        "C:\LowLatencyTrading\backup"
    )
    
    foreach ($dir in $prodDirs) {
        if (-not (Test-Path $dir)) {
            New-Item -ItemType Directory -Path $dir -Force
            Write-Host "Created directory: $dir" -ForegroundColor Green
        }
    }
    
    # Set up Windows Performance Toolkit (optional)
    Write-Host "Setting up high-performance configuration..." -ForegroundColor Yellow
    
    # Disable Windows Defender real-time protection for trading directory (requires admin)
    try {
        Add-MpPreference -ExclusionPath "C:\LowLatencyTrading" -ErrorAction SilentlyContinue
        Write-Host "Added Windows Defender exclusion for trading directory" -ForegroundColor Green
    }
    catch {
        Write-Warning "Could not add Windows Defender exclusion (requires admin privileges)"
    }
    
    # Set high performance power plan
    try {
        powercfg /setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c
        Write-Host "Set high performance power plan" -ForegroundColor Green
    }
    catch {
        Write-Warning "Could not set power plan (requires admin privileges)"
    }
    
    Write-Host "Production environment configured!" -ForegroundColor Green
}

Write-Host "`n=== Production Setup Summary ===" -ForegroundColor Green

if ($InstallDependencies) {
    Write-Host "✓ Dependencies installed via vcpkg" -ForegroundColor White
}
if ($SetupAeron) {
    Write-Host "✓ Aeron setup initiated (manual build required)" -ForegroundColor White
}
if ($ConfigureEnvironment) {
    Write-Host "✓ Production environment configured" -ForegroundColor White
}

Write-Host "`nNext Steps:" -ForegroundColor Yellow
Write-Host "1. Build Aeron C++ client (see BUILD_INSTRUCTIONS.txt)" -ForegroundColor White
Write-Host "2. Update CMakeLists.txt to use real Aeron" -ForegroundColor White
Write-Host "3. Build production version of trading system" -ForegroundColor White
Write-Host "4. Run production validation tests" -ForegroundColor White

Write-Host "`nUsage Examples:" -ForegroundColor Cyan
Write-Host "  .\production_setup.ps1 -All                    # Complete setup" -ForegroundColor Gray
Write-Host "  .\production_setup.ps1 -InstallDependencies    # Only install deps" -ForegroundColor Gray
Write-Host "  .\production_setup.ps1 -SetupAeron             # Only setup Aeron" -ForegroundColor Gray
Write-Host "  .\production_setup.ps1 -ConfigureEnvironment   # Only configure env" -ForegroundColor Gray 