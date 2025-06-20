# Simple Development Tools Installation Script
Write-Host "=== 低延迟交易系统 - 简化安装脚本 ===" -ForegroundColor Green

# Check if Chocolatey is installed
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "安装Chocolatey..." -ForegroundColor Yellow
    try {
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
        iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
        Write-Host "Chocolatey安装成功!" -ForegroundColor Green
    }
    catch {
        Write-Host "Chocolatey安装失败，请手动安装" -ForegroundColor Red
        Write-Host "访问: https://chocolatey.org/install" -ForegroundColor Yellow
        exit 1
    }
} else {
    Write-Host "Chocolatey已安装" -ForegroundColor Green
}

# Install basic tools
Write-Host "安装基础开发工具..." -ForegroundColor Yellow

$tools = @("git", "cmake")
foreach ($tool in $tools) {
    try {
        choco install $tool -y
        Write-Host "✓ $tool 安装成功" -ForegroundColor Green
    }
    catch {
        Write-Host "✗ $tool 安装失败" -ForegroundColor Red
    }
}

# Try to install MinGW (simpler than Visual Studio)
Write-Host "安装MinGW编译器..." -ForegroundColor Yellow
try {
    choco install mingw -y
    Write-Host "✓ MinGW 安装成功" -ForegroundColor Green
}
catch {
    Write-Host "✗ MinGW 安装失败" -ForegroundColor Red
}

Write-Host "`n安装完成!" -ForegroundColor Green
Write-Host "请重新启动PowerShell以刷新环境变量" -ForegroundColor Yellow

# Verification
Write-Host "`n验证安装:" -ForegroundColor Cyan
$commands = @("git --version", "cmake --version", "g++ --version")
foreach ($cmd in $commands) {
    try {
        $result = Invoke-Expression $cmd 2>$null
        Write-Host "✓ $cmd" -ForegroundColor Green
    }
    catch {
        Write-Host "✗ $cmd" -ForegroundColor Red
    }
} 