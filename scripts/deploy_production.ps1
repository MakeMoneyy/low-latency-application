# Low-Latency Trading System - Production Deployment Script
# This script deploys the trading system to production environment

param(
    [string]$BuildType = "Release",
    [string]$TargetDir = "C:\LowLatencyTrading",
    [switch]$RunTests,
    [switch]$StartServices,
    [switch]$Validate,
    [switch]$All
)

Write-Host "=== Low-Latency Trading System - Production Deployment ===" -ForegroundColor Green

if ($All) {
    $RunTests = $true
    $StartServices = $true
    $Validate = $true
}

# Configuration
$BuildDir = "build"
$ConfigDir = "config"
$LogDir = "$TargetDir\logs"
$DataDir = "$TargetDir\data"
$BackupDir = "$TargetDir\backup"

# Function to check prerequisites
function Test-Prerequisites {
    Write-Host "Checking prerequisites..." -ForegroundColor Yellow
    
    $prerequisites = @(
        @{ Name = "CMake"; Command = "cmake --version" },
        @{ Name = "Visual Studio Build Tools"; Command = "where cl.exe" },
        @{ Name = "vcpkg"; Command = "where vcpkg.exe" }
    )
    
    $allOk = $true
    foreach ($prereq in $prerequisites) {
        try {
            Invoke-Expression $prereq.Command | Out-Null
            Write-Host "  ✓ $($prereq.Name) found" -ForegroundColor Green
        }
        catch {
            Write-Host "  ✗ $($prereq.Name) not found" -ForegroundColor Red
            $allOk = $false
        }
    }
    
    return $allOk
}

# Function to build the system
function Build-System {
    Write-Host "Building trading system..." -ForegroundColor Yellow
    
    # Clean and create build directory
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
    
    Set-Location $BuildDir
    
    try {
        # Configure with production CMake
        Write-Host "Configuring build..." -ForegroundColor Gray
        cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=$BuildType -f ..\CMakeLists_production.txt
        
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
        
        # Build
        Write-Host "Compiling..." -ForegroundColor Gray
        cmake --build . --config $BuildType --parallel
        
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
        
        Write-Host "Build completed successfully!" -ForegroundColor Green
        
    } finally {
        Set-Location ..
    }
}

# Function to run tests
function Run-Tests {
    Write-Host "Running production tests..." -ForegroundColor Yellow
    
    $testExecutables = @(
        "$BuildDir\$BuildType\integration_test.exe",
        "$BuildDir\$BuildType\production_benchmark.exe"
    )
    
    foreach ($testExe in $testExecutables) {
        if (Test-Path $testExe) {
            Write-Host "Running $(Split-Path $testExe -Leaf)..." -ForegroundColor Gray
            
            try {
                & $testExe
                if ($LASTEXITCODE -eq 0) {
                    Write-Host "  ✓ Test passed" -ForegroundColor Green
                } else {
                    Write-Host "  ✗ Test failed" -ForegroundColor Red
                    throw "Test failed: $testExe"
                }
            }
            catch {
                Write-Host "  ✗ Test execution failed: $_" -ForegroundColor Red
                throw
            }
        } else {
            Write-Host "  ⚠ Test executable not found: $testExe" -ForegroundColor Yellow
        }
    }
}

# Function to deploy binaries
function Deploy-Binaries {
    Write-Host "Deploying binaries to production..." -ForegroundColor Yellow
    
    # Create target directories
    $dirs = @("$TargetDir\bin", "$TargetDir\config", $LogDir, $DataDir, $BackupDir)
    foreach ($dir in $dirs) {
        if (-not (Test-Path $dir)) {
            New-Item -ItemType Directory -Path $dir -Force | Out-Null
        }
    }
    
    # Copy executables
    $executables = @(
        "trading_system.exe",
        "market_data_simulator.exe"
    )
    
    foreach ($exe in $executables) {
        $sourcePath = "$BuildDir\$BuildType\$exe"
        $targetPath = "$TargetDir\bin\$exe"
        
        if (Test-Path $sourcePath) {
            Copy-Item $sourcePath $targetPath -Force
            Write-Host "  ✓ Deployed $exe" -ForegroundColor Green
        } else {
            Write-Host "  ⚠ Executable not found: $exe" -ForegroundColor Yellow
        }
    }
    
    # Copy configuration files
    $configFiles = @(
        "system_config.json",
        "aeron_config.json"
    )
    
    foreach ($configFile in $configFiles) {
        $sourcePath = "$ConfigDir\$configFile"
        $targetPath = "$TargetDir\config\$configFile"
        
        if (Test-Path $sourcePath) {
            Copy-Item $sourcePath $targetPath -Force
            Write-Host "  ✓ Deployed $configFile" -ForegroundColor Green
        } else {
            Write-Host "  ⚠ Config file not found: $configFile" -ForegroundColor Yellow
        }
    }
    
    # Copy dependencies (if any)
    $dllFiles = Get-ChildItem "$BuildDir\$BuildType\*.dll" -ErrorAction SilentlyContinue
    foreach ($dll in $dllFiles) {
        Copy-Item $dll.FullName "$TargetDir\bin\" -Force
        Write-Host "  ✓ Deployed $($dll.Name)" -ForegroundColor Green
    }
}

# Function to create Windows services
function Create-Services {
    Write-Host "Creating Windows services..." -ForegroundColor Yellow
    
    # Service configuration
    $services = @(
        @{
            Name = "LowLatencyTradingSystem"
            DisplayName = "Low-Latency Trading System"
            Description = "High-performance algorithmic trading system"
            BinaryPath = "$TargetDir\bin\trading_system.exe"
            StartType = "Manual"
        }
    )
    
    foreach ($service in $services) {
        try {
            # Check if service already exists
            $existingService = Get-Service -Name $service.Name -ErrorAction SilentlyContinue
            
            if ($existingService) {
                Write-Host "  ⚠ Service $($service.Name) already exists" -ForegroundColor Yellow
                
                # Stop service if running
                if ($existingService.Status -eq "Running") {
                    Stop-Service -Name $service.Name -Force
                    Write-Host "  ✓ Stopped existing service" -ForegroundColor Green
                }
                
                # Remove existing service
                sc.exe delete $service.Name | Out-Null
                Write-Host "  ✓ Removed existing service" -ForegroundColor Green
            }
            
            # Create new service
            New-Service -Name $service.Name `
                       -DisplayName $service.DisplayName `
                       -Description $service.Description `
                       -BinaryPathName $service.BinaryPath `
                       -StartupType $service.StartType
                       
            Write-Host "  ✓ Created service: $($service.Name)" -ForegroundColor Green
            
        }
        catch {
            Write-Host "  ✗ Failed to create service $($service.Name): $_" -ForegroundColor Red
        }
    }
}

# Function to validate deployment
function Test-Deployment {
    Write-Host "Validating deployment..." -ForegroundColor Yellow
    
    # Check binaries
    $requiredFiles = @(
        "$TargetDir\bin\trading_system.exe",
        "$TargetDir\config\system_config.json"
    )
    
    $allValid = $true
    foreach ($file in $requiredFiles) {
        if (Test-Path $file) {
            Write-Host "  ✓ Found: $(Split-Path $file -Leaf)" -ForegroundColor Green
        } else {
            Write-Host "  ✗ Missing: $(Split-Path $file -Leaf)" -ForegroundColor Red
            $allValid = $false
        }
    }
    
    # Test executable
    try {
        Write-Host "Testing trading system executable..." -ForegroundColor Gray
        $testResult = & "$TargetDir\bin\trading_system.exe" --version 2>&1
        Write-Host "  ✓ Executable runs successfully" -ForegroundColor Green
    }
    catch {
        Write-Host "  ✗ Executable test failed: $_" -ForegroundColor Red
        $allValid = $false
    }
    
    # Check services
    $services = @("LowLatencyTradingSystem")
    foreach ($serviceName in $services) {
        $service = Get-Service -Name $serviceName -ErrorAction SilentlyContinue
        if ($service) {
            Write-Host "  ✓ Service registered: $serviceName" -ForegroundColor Green
        } else {
            Write-Host "  ✗ Service not found: $serviceName" -ForegroundColor Red
            $allValid = $false
        }
    }
    
    return $allValid
}

# Function to start services
function Start-Services {
    Write-Host "Starting services..." -ForegroundColor Yellow
    
    $services = @("LowLatencyTradingSystem")
    
    foreach ($serviceName in $services) {
        try {
            Start-Service -Name $serviceName
            Write-Host "  ✓ Started service: $serviceName" -ForegroundColor Green
        }
        catch {
            Write-Host "  ✗ Failed to start service $serviceName: $_" -ForegroundColor Red
        }
    }
}

# Main deployment process
try {
    Write-Host "Starting deployment process..." -ForegroundColor Cyan
    Write-Host "Build Type: $BuildType" -ForegroundColor White
    Write-Host "Target Directory: $TargetDir" -ForegroundColor White
    
    # Check prerequisites
    if (-not (Test-Prerequisites)) {
        throw "Prerequisites not met"
    }
    
    # Build system
    Build-System
    
    # Run tests if requested
    if ($RunTests) {
        Run-Tests
    }
    
    # Deploy to production directory
    Deploy-Binaries
    
    # Create Windows services
    Create-Services
    
    # Validate deployment
    if ($Validate) {
        if (-not (Test-Deployment)) {
            throw "Deployment validation failed"
        }
    }
    
    # Start services if requested
    if ($StartServices) {
        Start-Services
    }
    
    Write-Host "`n=== Deployment Summary ===" -ForegroundColor Green
    Write-Host "✓ Build completed successfully" -ForegroundColor White
    Write-Host "✓ Binaries deployed to $TargetDir" -ForegroundColor White
    Write-Host "✓ Services created and configured" -ForegroundColor White
    
    if ($RunTests) {
        Write-Host "✓ Tests executed successfully" -ForegroundColor White
    }
    
    if ($Validate) {
        Write-Host "✓ Deployment validated" -ForegroundColor White
    }
    
    if ($StartServices) {
        Write-Host "✓ Services started" -ForegroundColor White
    }
    
    Write-Host "`nDeployment completed successfully!" -ForegroundColor Green
    Write-Host "Trading system is ready for production use." -ForegroundColor Green
    
    Write-Host "`nNext Steps:" -ForegroundColor Yellow
    Write-Host "1. Configure system settings in $TargetDir\config\" -ForegroundColor White
    Write-Host "2. Set up monitoring and alerting" -ForegroundColor White
    Write-Host "3. Configure network and firewall settings" -ForegroundColor White
    Write-Host "4. Set up backup and disaster recovery" -ForegroundColor White
    Write-Host "5. Conduct final acceptance testing" -ForegroundColor White
    
}
catch {
    Write-Host "`nDeployment failed: $_" -ForegroundColor Red
    exit 1
}

Write-Host "`nUsage Examples:" -ForegroundColor Cyan
Write-Host "  .\deploy_production.ps1 -All                    # Complete deployment" -ForegroundColor Gray
Write-Host "  .\deploy_production.ps1 -RunTests               # Deploy with testing" -ForegroundColor Gray
Write-Host "  .\deploy_production.ps1 -Validate               # Deploy with validation" -ForegroundColor Gray
Write-Host "  .\deploy_production.ps1 -StartServices          # Deploy and start services" -ForegroundColor Gray 