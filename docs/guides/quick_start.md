# Sylar框架快速入门指南

## 1. 环境准备

### 1.1 系统要求
- Linux/Unix系统（推荐Ubuntu 18.04+）
- C++11兼容的编译器（GCC 4.8+ 或 Clang）
- CMake 3.10+
- yaml-cpp开发库

### 1.2 依赖安装（Ubuntu/Debian）
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libyaml-cpp-dev
```

## 2. 编译项目

### 2.1 克隆项目
```bash
git clone <项目地址>
cd sylar
```

### 2.2 构建项目
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

编译完成后，输出文件位于：
- 动态库: `lib/libsylar.so`
- 可执行文件: `bin/`

## 3. 运行测试

编译完成后，可以运行测试程序验证功能：

```bash
# 运行日志基础测试
./bin/test_log_basic

# 运行协程测试
./bin/test_coroutine

# 运行网络地址测试
./bin/test_address
```

## 4. 配置文件

示例配置文件位于 `bin/config/` 目录：
- `log.yaml` - 日志配置示例
- `test.yaml` - 测试配置示例

## 5. 开发示例

### 5.1 简单日志使用
```cpp
#include "log/logger.hpp"

auto logger = SYLAR_LOG_ROOT();
SYLAR_LOG_INFO(logger) << "Hello, Sylar!";
```

### 5.2 协程使用
```cpp
#include "runtime/coroutine.hpp"

void coroutine_func() {
    SYLAR_LOG_INFO(g_logger) << "In coroutine";
}

// 创建并运行协程
sylar::Coroutine::ptr coro = std::make_shared<sylar::Coroutine>(coroutine_func);
coro->swapIn();
```

### 5.3 网络编程
```cpp
#include "net/socket.hpp"
#include "net/address.hpp"

// 创建TCP服务器
auto addr = sylar::Address::LookupAnyIPAddress("0.0.0.0:8080");
sylar::Socket::ptr sock = sylar::Socket::CreateTCPSocket();
sock->bind(addr);
sock->listen();
```

## 6. 项目结构

```
sylar/
├── include/          # 头文件
├── src/              # 源文件
├── tests/            # 测试文件
├── bin/              # 可执行文件和配置
├── lib/              # 库文件
├── docs/             # 文档
└── build/            # 构建目录
```

## 7. 进一步学习

- 查看API文档: `docs/api/`
- 了解系统架构: `docs/architecture/`
- 性能测试报告: `docs/performance/`