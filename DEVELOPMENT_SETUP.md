# 低延迟交易系统 - 开发环境设置指南

## 当前状态
✅ 生产环境配置完成  
✅ 核心代码和测试完成  
⚠️ 需要安装开发工具链  

## 必需的开发工具

### 1. Visual Studio 2019 或更高版本
**推荐选项**: Visual Studio Community 2019 (免费)

**下载链接**: https://visualstudio.microsoft.com/zh-hans/downloads/

**安装组件**:
- ✅ C++ 桌面开发工作负载
- ✅ CMake 工具
- ✅ Git for Windows
- ✅ Windows 10/11 SDK

**安装命令** (使用Chocolatey):
```powershell
# 安装Chocolatey (如果未安装)
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# 安装Visual Studio
choco install visualstudio2019community --params "--add Microsoft.VisualStudio.Workload.NativeDesktop --add Microsoft.VisualStudio.Component.VC.CMake.Project"
```

### 2. 替代选项: MinGW-w64 + CMake
如果不想安装完整的Visual Studio：

```powershell
# 使用Chocolatey安装
choco install mingw cmake git
```

### 3. vcpkg 包管理器
```powershell
# 克隆vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# 添加到系统PATH
$env:PATH += ";C:\vcpkg"
[Environment]::SetEnvironmentVariable("PATH", $env:PATH, [EnvironmentVariableTarget]::Machine)
```

## 快速安装脚本

创建并运行以下PowerShell脚本 (需要管理员权限):

```powershell
# install_dev_tools.ps1
Write-Host "安装低延迟交易系统开发环境..." -ForegroundColor Green

# 检查管理员权限
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "请以管理员身份运行此脚本" -ForegroundColor Red
    exit 1
}

# 安装Chocolatey
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Host "安装Chocolatey..." -ForegroundColor Yellow
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
}

# 安装开发工具
Write-Host "安装开发工具..." -ForegroundColor Yellow
choco install -y git cmake mingw visualstudio2019buildtools

# 安装vcpkg
if (-not (Test-Path "C:\vcpkg")) {
    Write-Host "安装vcpkg..." -ForegroundColor Yellow
    git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
    Set-Location C:\vcpkg
    .\bootstrap-vcpkg.bat
    .\vcpkg integrate install
}

Write-Host "开发环境安装完成!" -ForegroundColor Green
```

## 验证安装

安装完成后，重新打开PowerShell并运行：

```powershell
# 验证工具安装
cmake --version
git --version
cl  # 或者 g++ --version (如果使用MinGW)

# 验证vcpkg
C:\vcpkg\vcpkg version
```

## 下一步操作

安装完开发工具后，继续以下步骤：

### 1. 安装C++依赖包
```powershell
# 使用vcpkg安装依赖
C:\vcpkg\vcpkg install spdlog:x64-windows
C:\vcpkg\vcpkg install nlohmann-json:x64-windows
C:\vcpkg\vcpkg install fmt:x64-windows
```

### 2. 构建Aeron库
```powershell
# 重新下载Aeron (如果之前下载失败)
.\scripts\production_setup.ps1 -SetupAeron

# 构建Aeron
cd C:\aeron\aeron-1.44.1
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 3. 构建交易系统
```powershell
# 返回项目目录
cd "D:\low-latency application"

# 清理并重新构建
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
mkdir build
cd build

# 使用生产配置构建
cmake .. -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build . --config Release
```

### 4. 运行生产测试
```powershell
# 运行基准测试
.\Release\production_benchmark.exe

# 运行集成测试
.\Release\integration_test.exe
```

## 故障排除

### 常见问题

1. **"找不到编译器"**
   - 确保安装了Visual Studio Build Tools或MinGW
   - 重新启动PowerShell以刷新PATH

2. **"CMake配置失败"**
   - 检查CMake版本 (需要3.10+)
   - 确保vcpkg正确集成

3. **"找不到依赖库"**
   - 运行 `vcpkg list` 检查已安装的包
   - 确保使用了正确的triplet (x64-windows)

### 获取帮助

如果遇到问题，请检查：
1. Windows事件日志
2. 编译器错误输出
3. CMake配置日志

## 预期结果

安装完成后，您应该能够：
- ✅ 构建完整的交易系统
- ✅ 运行所有测试程序
- ✅ 集成真实的Aeron库
- ✅ 部署到生产环境

**估计安装时间**: 30-60分钟 (取决于网络速度) 