# 低延迟交易系统 (Low Latency Trading System)

基于C++和Aeron实现的高性能低延迟交易系统，采用方向性变化（Directional Change, DC）策略。

## 项目概述

本项目实现了一个完整的低延迟交易系统，包含三个核心组件：
- **市场数据处理器** (Market Data Processor): 处理实时市场数据，检测DC事件
- **策略引擎** (Strategy Engine): 基于DC指标生成交易信号
- **执行引擎** (Execution Engine): 执行交易订单并跟踪性能

系统通过Aeron消息传递框架实现微秒级延迟的组件间通信。

## 系统架构

```
Market Data → DC Events → Trading Signals → Order Execution
     ↓              ↓              ↓              ↓
[市场数据处理器] → [策略引擎] → [执行引擎] → [性能跟踪]
```

## 核心特性

### 交易策略
- **DC事件检测**: 基于可配置阈值(theta)检测价格趋势变化
- **DC指标计算**: 
  - TMV_EXT(n): 总价格运动范围
  - T(n): 趋势完成时间  
  - R(n): 时间调整回报率
- **HMM集成**: 可选的隐马尔可夫模型用于市场状态检测

### 性能优化
- **微秒级延迟**: 目标延迟 <100微秒 (单机环境)
- **高吞吐量**: 支持>20M消息/秒处理能力
- **无锁设计**: 采用原子操作和lock-free数据结构
- **CPU亲和性**: 支持CPU核心绑定优化

### 监控与评估
- **实时延迟监控**: 纳秒级精度的延迟测量
- **交易绩效指标**: PnL、胜率、夏普比率、最大回撤等
- **详细日志记录**: 使用spdlog进行高性能日志记录

## 系统要求

### 硬件要求
- CPU: 支持高频交易的多核处理器
- 内存: 至少8GB RAM
- 网络: 低延迟网络连接 (如果需要跨主机通信)

### 软件依赖
- **操作系统**: Linux (推荐) 或 Windows
- **编译器**: GCC 7+ 或 Clang 6+ (支持C++17)
- **必需库**:
  - Aeron C++ (消息传递)
  - spdlog (日志记录)
  - nlohmann/json (配置文件解析)

## 构建与安装

### 1. 安装依赖

#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake git
sudo apt-get install libaeron-dev libspdlog-dev nlohmann-json3-dev
```

#### CentOS/RHEL:
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake3 git
# 需要手动编译安装Aeron和其他依赖
```

### 2. 编译项目
```bash
git clone <repository-url>
cd low-latency-trading-system
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 3. 配置系统
编辑 `config/system_config.json` 文件调整系统参数：
```json
{
  "dc_strategy": {
    "theta": 0.004,
    "enable_tmv_calculation": true
  },
  "execution": {
    "simulation_mode": true,
    "initial_capital": 100000.0
  }
}
```

## 使用方法

### 启动完整系统
```bash
./trading_system config/system_config.json
```

### 独立启动各组件
```bash
# 启动市场数据处理器
./market_data_processor

# 启动策略引擎  
./strategy_engine

# 启动执行引擎
./execution_engine
```

## 配置说明

### DC策略参数
- `theta`: DC阈值，默认0.004 (0.4%)
- `enable_tmv_calculation`: 是否启用TMV计算
- `enable_time_adjustment`: 是否启用时间调整

### Aeron配置
- `channel`: 通信通道 (如 "aeron:ipc" 用于本机通信)
- `stream_id`: 流标识符
- `directory`: Aeron媒体驱动目录

### 性能配置
- `enable_latency_tracking`: 启用延迟跟踪
- `latency_report_interval_ms`: 延迟报告间隔

## 性能调优

### 系统级优化
```bash
# 设置CPU调度策略
sudo sysctl kernel.sched_rt_runtime_us=-1

# 禁用CPU频率调节
sudo cpupower frequency-set -g performance

# 设置网络缓冲区
sudo sysctl net.core.rmem_max=134217728
sudo sysctl net.core.wmem_max=134217728
```

### 应用级优化
- 使用 `BusySpinIdleStrategy` 用于最低延迟
- 绑定线程到特定CPU核心
- 启用CPU亲和性设置
- 调整GC设置 (如果使用Java组件)

## 测试

### 单元测试
```bash
cd build
make test
```

### 压力测试
```bash
# 运行延迟测试
./latency_test

# 运行吞吐量测试  
./throughput_test
```

### 回测
```bash
# 使用历史数据进行回测
./backtesting --data historical_data.csv --config backtest_config.json
```

## 监控与诊断

### 实时监控
系统提供实时统计信息，包括：
- 处理的消息数量
- 检测到的DC事件数
- 平均和最大延迟
- 交易执行统计

### 日志分析
```bash
# 查看系统日志
tail -f trading_system.log

# 分析延迟分布
grep "latency" trading_system.log | awk '{print $NF}' | sort -n
```

## API文档

使用Doxygen生成API文档：
```bash
doxygen Doxyfile
# 文档生成在 docs/html/ 目录
```

## 风险管理

### 内置风险控制
- 最大持仓限制
- 单笔交易风险限制  
- 回撤控制机制
- 异常检测和自动停止

### 建议的风险管理实践
- 定期备份交易日志
- 监控系统资源使用
- 设置交易限额
- 实施熔断机制

## 故障排除

### 常见问题

1. **Aeron连接失败**
   - 检查Aeron媒体驱动是否运行
   - 验证通道配置和权限

2. **高延迟问题**
   - 检查CPU频率调节设置
   - 验证线程亲和性配置
   - 监控系统负载

3. **内存泄漏**
   - 使用Valgrind检测内存泄漏
   - 检查循环引用

## 贡献指南

1. Fork项目仓库
2. 创建特性分支
3. 提交更改
4. 创建Pull Request

## 许可证

本项目采用MIT许可证。详情请参见LICENSE文件。

## 联系方式

如有问题或建议，请联系项目维护者。

---

**注意**: 本系统仅用于教育和研究目的。在实际交易环境中使用前，请进行充分的测试和风险评估。 