# Low-Latency Trading System - Development Tools Installation Script
# This script installs all necessary development tools for building the trading system

param(
    [switch]$InstallChocolatey,
    [switch]$InstallCompiler,
    [switch]$InstallVcpkg,
    [switch]$InstallDependencies,
    [switch]$All
)

Write-Host "=== 低延迟交易系统 - 开发环境安装 ===" -ForegroundColor Green

if ($All) {
    $InstallChocolatey = $true
    $InstallCompiler = $true
    $InstallVcpkg = $true
    $InstallDependencies = $true
}

# Function to check if running as administrator
function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Function to install Chocolatey
function Install-Chocolatey {
    Write-Host "`n=== 安装Chocolatey包管理器 ===" -ForegroundColor Cyan
    
    if (Get-Command choco -ErrorAction SilentlyContinue) {
        Write-Host "Chocolatey已安装" -ForegroundColor Green
        return
    }
    
    try {
        Write-Host "下载并安装Chocolatey..." -ForegroundColor Yellow
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
        
        # Refresh environment variables
        $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("PATH","User")
        
        Write-Host "Chocolatey安装成功!" -ForegroundColor Green
    }
    catch {
        Write-Error "Chocolatey安装失败: $_"
        throw
    }
}

# Function to install compiler tools
function Install-CompilerTools {
    Write-Host "`n=== 安装编译工具 ===" -ForegroundColor Cyan
    
    # Check if Visual Studio is already installed
    $vsInstalled = Get-Command cl -ErrorAction SilentlyContinue
    if ($vsInstalled) {
        Write-Host "Visual Studio编译器已安装" -ForegroundColor Green
        return
    }
    
    Write-Host "安装开发工具..." -ForegroundColor Yellow
    
    try {
        # Install essential tools
        choco install -y git
        choco install -y cmake
        
        # Try to install Visual Studio Build Tools first
        Write-Host "尝试安装Visual Studio Build Tools..." -ForegroundColor Yellow
        choco install -y visualstudio2019buildtools --params "--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.CMake.Project"
        
        # If that fails, install MinGW as fallback
        if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
            Write-Host "安装MinGW作为备选编译器..." -ForegroundColor Yellow
            choco install -y mingw
        }
        
        Write-Host "编译工具安装完成!" -ForegroundColor Green
        
        # Refresh PATH
        $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("PATH","User")
        
    }
    catch {
        Write-Error "编译工具安装失败: $_"
        throw
    }
}

# Function to install vcpkg
function Install-Vcpkg {
    Write-Host "`n=== 安装vcpkg包管理器 ===" -ForegroundColor Cyan
    
    $vcpkgPath = "C:\vcpkg"
    
    if (Test-Path "$vcpkgPath\vcpkg.exe") {
        Write-Host "vcpkg已安装" -ForegroundColor Green
        return
    }
    
    try {
        Write-Host "克隆vcpkg仓库..." -ForegroundColor Yellow
        
        if (Test-Path $vcpkgPath) {
            Remove-Item -Recurse -Force $vcpkgPath
        }
        
        git clone https://github.com/Microsoft/vcpkg.git $vcpkgPath
        
        Write-Host "构建vcpkg..." -ForegroundColor Yellow
        Set-Location $vcpkgPath
        .\bootstrap-vcpkg.bat
        
        Write-Host "集成vcpkg到Visual Studio..." -ForegroundColor Yellow
        .\vcpkg integrate install
        
        # Add to PATH
        $currentPath = [System.Environment]::GetEnvironmentVariable("PATH", "Machine")
        if ($currentPath -notlike "*$vcpkgPath*") {
            [System.Environment]::SetEnvironmentVariable("PATH", "$currentPath;$vcpkgPath", "Machine")
        }
        
        Write-Host "vcpkg安装完成!" -ForegroundColor Green
        
    }
    catch {
        Write-Error "vcpkg安装失败: $_"
        throw
    }
    finally {
        Set-Location $PSScriptRoot
    }
}

# Function to install C++ dependencies
function Install-Dependencies {
    Write-Host "`n=== 安装C++依赖库 ===" -ForegroundColor Cyan
    
    $vcpkgPath = "C:\vcpkg\vcpkg.exe"
    
    if (-not (Test-Path $vcpkgPath)) {
        Write-Error "vcpkg未找到，请先安装vcpkg"
        return
    }
    
    $packages = @(
        "spdlog:x64-windows",
        "nlohmann-json:x64-windows", 
        "fmt:x64-windows",
        "gtest:x64-windows"
    )
    
    foreach ($package in $packages) {
        Write-Host "安装 $package..." -ForegroundColor Yellow
        try {
            & $vcpkgPath install $package
            Write-Host "✓ $package 安装成功" -ForegroundColor Green
        }
        catch {
            Write-Warning "安装 $package 失败: $_"
        }
    }
    
    Write-Host "依赖库安装完成!" -ForegroundColor Green
}

# Function to verify installation
function Test-Installation {
    Write-Host "`n=== 验证安装 ===" -ForegroundColor Cyan
    
    $tools = @(
        @{ Name = "Git"; Command = "git --version" },
        @{ Name = "CMake"; Command = "cmake --version" },
        @{ Name = "vcpkg"; Command = "C:\vcpkg\vcpkg.exe version" }
    )
    
    # Check compiler
    if (Get-Command cl -ErrorAction SilentlyContinue) {
        $tools += @{ Name = "Visual Studio Compiler"; Command = "cl" }
    } elseif (Get-Command g++ -ErrorAction SilentlyContinue) {
        $tools += @{ Name = "MinGW Compiler"; Command = "g++ --version" }
    }
    
    $allOk = $true
    foreach ($tool in $tools) {
        try {
            $output = Invoke-Expression $tool.Command 2>$null
            Write-Host "✓ $($tool.Name): 可用" -ForegroundColor Green
        }
        catch {
            Write-Host "✗ $($tool.Name): 不可用" -ForegroundColor Red
            $allOk = $false
        }
    }
    
    return $allOk
}

# Main installation process
try {
    Write-Host "开始安装开发环境..." -ForegroundColor White
    
    # Check admin privileges
    if (-not (Test-Administrator)) {
        Write-Warning "建议以管理员身份运行此脚本以获得最佳效果"
        Write-Host "某些操作可能需要管理员权限" -ForegroundColor Yellow
    }
    
    # Install components based on parameters
    if ($InstallChocolatey -or $All) {
        Install-Chocolatey
    }
    
    if ($InstallCompiler -or $All) {
        Install-CompilerTools
    }
    
    if ($InstallVcpkg -or $All) {
        Install-Vcpkg
    }
    
    if ($InstallDependencies -or $All) {
        Install-Dependencies
    }
    
    # Verify installation
    Write-Host "`n=== 安装验证 ===" -ForegroundColor Green
    $success = Test-Installation
    
    if ($success) {
        Write-Host "`n🎉 开发环境安装成功!" -ForegroundColor Green
        Write-Host "您现在可以构建低延迟交易系统了。" -ForegroundColor White
        
        Write-Host "`n下一步操作:" -ForegroundColor Yellow
        Write-Host "1. 重新启动PowerShell以刷新环境变量" -ForegroundColor White
        Write-Host "2. 运行: .\scripts\production_setup.ps1 -SetupAeron" -ForegroundColor White
        Write-Host "3. 构建项目: cmake 和 编译" -ForegroundColor White
    } else {
        Write-Host "`n⚠️ 某些工具安装可能不完整" -ForegroundColor Yellow
        Write-Host "请检查上述错误信息并手动安装缺失的工具" -ForegroundColor White
    }
    
}
catch {
    Write-Host "`n❌ 安装过程中发生错误: $_" -ForegroundColor Red
    Write-Host "请检查错误信息并重试" -ForegroundColor White
    exit 1
}

Write-Host "`n使用示例:" -ForegroundColor Cyan
Write-Host "  .\install_dev_tools.ps1 -All                    # 完整安装" -ForegroundColor Gray
Write-Host "  .\install_dev_tools.ps1 -InstallChocolatey      # 只安装Chocolatey" -ForegroundColor Gray
Write-Host "  .\install_dev_tools.ps1 -InstallCompiler        # 只安装编译器" -ForegroundColor Gray
Write-Host "  .\install_dev_tools.ps1 -InstallVcpkg           # 只安装vcpkg" -ForegroundColor Gray
Write-Host "  .\install_dev_tools.ps1 -InstallDependencies    # 只安装依赖" -ForegroundColor Gray 