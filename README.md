# Sylar C++ 高性能服务器框架

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![Language](https://img.shields.io/badge/language-C++11-blue)
![License](https://img.shields.io/badge/license-MIT-blue)

## 简介

Sylar是一个基于C++11的高性能服务框架，专注于提供日志、配置管理、协程调度、线程同步、定时器等基础组件，适用于高并发网络服务开发。

## 特性

- 🚀 **高性能**: 基于协程的异步I/O调度模型
- 📝 **日志系统**: 支持多级日志、格式化输出、多种Appender
- ⚙️  **配置管理**: 基于YAML的配置解析与动态更新
- 🧵 **线程同步**: 提供丰富的线程控制与同步机制
- ⏱️  **定时器**: 高精度定时任务管理
- 🧩 **模块化设计**: 高内聚低耦合的模块划分

## 核心模块

1. **日志系统** - 支持多级日志、格式化输出、多种Appender（如文件、控制台）、线程安全写入
2. **配置管理** - 基于YAML的配置解析，支持类型安全的变量注册与回调通知
3. **协程调度** - 提供Coroutine协程封装，支持Scheduler多线程调度和IOManager异步I/O事件驱动
4. **线程与同步** - 提供Thread、Semaphore、Lock等线程控制与同步机制
5. **定时器** - 实现高精度定时任务管理
6. **工具库** - 包含字符串转换（lexical_cast）、单例模式、不可拷贝基类等通用工具

## 快速开始

### 环境要求
- Linux/Unix系统
- C++11兼容的编译器
- CMake 3.10+
- yaml-cpp开发库

### 编译安装

#### 方法1: 使用自动构建脚本（推荐）
```bash
git clone <项目地址>
cd sylar
./autobuild.sh
```

#### 方法2: 手动编译
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 运行测试
```bash
# 运行日志基础测试
./bin/test_log_basic

# 运行协程测试
./bin/test_coroutine
```

## 构建优化

为了提高编译速度，项目提供了多种构建优化选项：

### 1. 增量编译
```bash
# 增量编译，避免重新编译未更改的文件
./autobuild.sh Debug true
```

### 2. 使用ccache缓存编译结果
```bash
# 需要先安装ccache: sudo apt-get install ccache
./ccache_build.sh
```

### 3. 使用Ninja构建系统
```bash
# 需要先安装ninja: sudo apt-get install ninja-build
./ninja_build.sh
```

## 配置文件

项目包含示例配置文件，位于 `examples/config/` 目录：
- `log.yaml` - 日志配置示例
- `test.yaml` - 测试配置示例

使用时需要将配置文件复制到 `bin/config/` 目录中。

## 文档

- [快速入门](docs/guides/quick_start.md)
- [系统架构](docs/architecture/system_arch.md)
- [模块设计](docs/architecture/module_design.md)
- [API文档](docs/api/)
- [性能测试报告](docs/performance/)

## 示例

### 简单日志使用
```cpp
#include "log/logger.hpp"

auto logger = SYLAR_LOG_ROOT();
SYLAR_LOG_INFO(logger) << "Hello, Sylar!";
```

### 协程使用
```cpp
#include "runtime/coroutine.hpp"

void coroutine_func() {
    SYLAR_LOG_INFO(g_logger) << "In coroutine";
}

// 创建并运行协程
sylar::Coroutine::ptr coro = std::make_shared<sylar::Coroutine>(coroutine_func);
coro->swapIn();
```

## 贡献

欢迎提交Issue和Pull Request来帮助改进Sylar框架。

## 许可证

本项目采用MIT许可证，详情请见[LICENSE](LICENSE)文件。