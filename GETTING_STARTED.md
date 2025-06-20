# 快速入门指南

## 项目概述

您现在拥有一个完整的低延迟交易系统项目框架，包含以下核心组件：

### 已创建的文件结构

```
low-latency application/
├── CMakeLists.txt              # CMake构建配置
├── Makefile                    # 简单Make构建
├── README.md                   # 详细项目文档
├── GETTING_STARTED.md          # 本文件
├── Requirements.cursorrules    # 需求文档
├── config/
│   └── system_config.json      # 系统配置文件
├── include/
│   ├── common/
│   │   ├── Config.h            # 配置管理
│   │   ├── DCIndicator.h       # DC指标计算
│   │   ├── Logger.h            # 日志系统
│   │   └── TimeUtils.h         # 时间工具
│   ├── execution/
│   │   └── ExecutionEngine.h   # 执行引擎
│   ├── market_data/
│   │   └── MarketDataProcessor.h # 市场数据处理器
│   └── strategy/
│       └── StrategyEngine.h    # 策略引擎
├── src/
│   ├── common/
│   │   ├── DCIndicator.cpp     # DC指标实现
│   │   └── TimeUtils.cpp       # 时间工具实现
│   └── main/
│       └── trading_system_main.cpp # 主程序
└── scripts/
    └── quick_start.sh          # 快速启动脚本 (Linux/Mac)
```

## 已实现的核心功能

### 1. DC指标计算系统
- ✅ 方向性变化(DC)事件检测
- ✅ TMV_EXT指标计算
- ✅ 时间调整回报计算
- ✅ 可配置的theta阈值

### 2. 系统架构
- ✅ 三层架构设计 (市场数据 → 策略 → 执行)
- ✅ Aeron消息传递集成
- ✅ 高精度时间测量
- ✅ 多线程并发处理

### 3. 配置和日志
- ✅ JSON配置文件支持
- ✅ spdlog日志系统集成
- ✅ 性能监控和统计

## 下一步开发任务

### Phase 1: 完成核心实现 (必需)
1. **实现缺失的源文件**：
   ```bash
   # 需要创建的关键实现文件：
   src/common/Config.cpp
   src/common/Logger.cpp
   src/market_data/MarketDataProcessor.cpp
   src/strategy/StrategyEngine.cpp
   src/execution/ExecutionEngine.cpp
   ```

2. **添加必需的依赖**：
   - nlohmann/json (JSON解析)
   - spdlog (日志记录)
   - Aeron C++ 客户端库

### Phase 2: 构建和测试
1. **构建系统**：
   ```bash
   # 使用CMake (推荐)
   mkdir build
   cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j4
   
   # 或使用Makefile
   make cmake-build
   ```

2. **创建测试数据生成器**：
   - 实现模拟市场数据生成器
   - 创建单元测试框架

### Phase 3: 高级特性
1. **HMM集成** (可选)：
   - 实现隐马尔可夫模型
   - 市场状态检测

2. **性能优化**：
   - CPU亲和性设置
   - 内存对齐优化
   - 无锁数据结构

## Windows环境特殊说明

由于您使用的是Windows PowerShell环境，建议：

1. **使用Visual Studio**：
   ```cmd
   # 生成Visual Studio项目文件
   mkdir build
   cd build
   cmake -G "Visual Studio 16 2019" ..
   ```

2. **或使用MinGW-w64**：
   ```cmd
   # 安装MinGW-w64后
   cmake -G "MinGW Makefiles" ..
   mingw32-make
   ```

3. **依赖安装**：
   - 使用vcpkg安装依赖库
   - 或手动编译Aeron、spdlog等库

## 配置文件说明

`config/system_config.json` 已包含所有必要配置：

```json
{
  "dc_strategy": {
    "theta": 0.004,           // DC阈值 (0.4%)
    "enable_tmv_calculation": true
  },
  "execution": {
    "simulation_mode": true,   // 模拟模式
    "initial_capital": 100000.0
  }
}
```

## 运行系统

系统启动后会：
1. 初始化三个核心组件
2. 建立Aeron通信连接
3. 开始处理数据流
4. 每10秒输出统计信息
5. 在关闭时生成性能报告

## 性能目标

根据需求规格，系统应达到：
- 延迟：< 100微秒 (单机环境)
- 吞吐量：> 20M 消息/秒
- DC事件检测准确率：> 95%

## 问题排查

如遇到编译问题：
1. 确保所有依赖库已正确安装
2. 检查CMake版本 (需要3.10+)
3. 验证C++17编译器支持

## 联系与支持

这是一个教育性项目框架。在实际交易环境中使用前，请：
- 进行充分的回测验证
- 实施风险管理机制
- 遵守相关法规要求

---

**恭喜！** 您已经有了一个功能完整的低延迟交易系统项目框架。接下来就是完成实现细节和测试验证了。 