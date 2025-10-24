# Sylar框架网络模块文档

## 概述

Sylar框架的网络模块提供了一套完整的网络编程解决方案，包括地址封装、Socket操作、流处理、TCP服务器等核心组件。该模块设计目标是提供高性能、易用性和可扩展性，支持IPv4、IPv6和Unix域套接字，并与框架的协程系统无缝集成。

## 模块结构

网络模块主要由以下几个核心组件构成：

1. [Address](#address模块) - 网络地址封装
2. [Socket](#socket模块) - Socket操作封装
3. [Stream](#stream模块) - 流操作接口
4. [SocketStream](#socketstream模块) - Socket流实现
5. [TCPServer](#tcpserver模块) - TCP服务器封装
6. [Hook](#hook模块) - 系统调用钩子
7. [FdManager](#fdmanager模块) - 文件描述符管理
8. [ByteArray](#bytearray模块) - 字节数组处理

## Address模块

Address模块提供了对各种网络地址类型的封装，包括IPv4、IPv6和Unix域套接字地址。

### 类结构

```
Address (基类)
├── IPAddress (IP地址接口)
│   ├── IPv4Address (IPv4地址实现)
│   └── IPv6Address (IPv6地址实现)
├── UnixAddress (Unix域套接字地址)
└── UnknownAddress (未知类型地址)
```

### Address基类

Address是所有地址类型的基类，定义了地址的基本操作接口：

- `Create` - 通过sockaddr指针创建Address对象
- `Lookup` - 通过主机名解析地址列表
- `getFamily` - 获取地址族
- `getAddr` - 获取sockaddr指针
- `getAddrLen` - 获取sockaddr长度
- 比较运算符（<, ==, !=）

### IPAddress类

IPAddress继承自Address，专门用于表示IP地址（IPv4和IPv6），提供了广播地址、网络地址、子网掩码等相关操作：

- `broadcastAddress` - 获取广播地址
- `networkAddress` - 获取网络地址
- `subnetMask` - 获取子网掩码地址
- `getPort` - 获取端口号
- `setPort` - 设置端口号

### IPv4Address类

IPv4Address是IPv4地址的具体实现，支持IPv4地址的各种操作。

### IPv6Address类

IPv6Address是IPv6地址的具体实现，支持IPv6地址的各种操作。

### UnixAddress类

UnixAddress用于表示Unix域套接字地址。

### UnknownAddress类

UnknownAddress用于表示未知类型的地址。

## Socket模块

Socket模块提供了对底层socket的高级封装，支持TCP、UDP协议以及IPv4、IPv6和Unix域套接字。

### Socket类

Socket类是Socket操作的核心类，提供以下主要功能：

#### 基本属性
- 协议族（IPv4、IPv6、Unix）
- 类型（TCP、UDP）
- 协议信息
- 本地地址和远程地址
- 连接状态

#### 工厂方法
- `CreateTCP` - 创建TCP Socket
- `CreateUDP` - 创建UDP Socket
- 针对不同地址族的创建方法

#### 核心操作
- `bind` - 绑定地址
- `connect` - 连接远程地址
- `listen` - 监听连接
- `accept` - 接受连接
- `send/recv` - 发送和接收数据
- `sendTo/recvFrom` - 面向无连接的数据传输
- `close` - 关闭Socket
- 超时设置和获取
- Socket选项操作

#### SSL支持
SSLSocket继承自Socket，提供基于SSL/TLS的安全网络通信功能。

## Stream模块

Stream模块定义了流操作的抽象接口，为数据的读写提供统一的接口。

### Stream类

Stream类定义了流的基本操作接口：

- `read` - 读取数据
- `write` - 写入数据
- `close` - 关闭流
- 固定长度读写的辅助方法

### SocketStream类

SocketStream是Stream的具体实现，基于Socket提供流式数据传输功能。

## TCPServer模块

TCPServer模块提供了TCP服务器的封装实现。

### TcpServer类

TcpServer类提供了TCP服务器的核心功能：

- 多地址绑定支持
- SSL支持
- 协程调度器集成
- 客户端连接处理
- 服务器启停控制

### TcpServerConf结构

TcpServerConf用于配置TCP服务器的各种参数：

- 地址列表
- Keepalive设置
- 超时设置
- SSL配置
- 工作协程调度器配置

## Hook模块

Hook模块实现了对系统调用的拦截，使其协程化，避免阻塞整个线程。

### 主要功能

- 检查和设置钩子功能启用状态
- 拦截常见的系统调用（sleep、socket相关、IO操作等）
- 使阻塞操作变为协程挂起和恢复

## FdManager模块

FdManager模块提供了文件描述符的统一管理机制。

### FdCtx类

FdCtx类管理单个文件描述符的上下文信息：

- 初始化状态
- Socket类型标识
- 阻塞模式设置
- 超时时间管理

### FdManager类

FdManager类是全局的文件描述符管理器：

- 线程安全的文件描述符上下文获取和删除
- 单例模式实现

### FileDescriptor类

FileDescriptor类提供了RAII风格的文件描述符封装，自动管理文件描述符的生命周期。

## ByteArray模块

ByteArray模块提供了对可变长度字节数据流的处理功能。

### 主要特性

- 链表结构管理内存块
- 支持多种数据类型的读写操作
- 固定长度和变长编码支持
- 大小端字节序转换
- 文件读写支持
- 网络传输优化（iovec支持）

### 核心功能

- 基本数据类型读写（整数、浮点数、字符串）
- 固定长度和变长编码整数读写
- 字符串读写（多种长度编码方式）
- 位置控制和容量管理
- 数据序列化和反序列化

## 使用示例

### 创建TCP服务器

```cpp
#include "net/tcp_server.hpp"
#include "net/socket.hpp"

// 创建TCP服务器
sylar::TcpServer::ptr server(new sylar::TcpServer);

// 创建并绑定地址
sylar::Address::ptr addr = sylar::Address::LookupAny("0.0.0.0:8080");
server->bind(addr);

// 启动服务器
server->start();
```

### 创建TCP客户端

```cpp
#include "net/socket.hpp"

// 创建TCP Socket
sylar::Socket::ptr sock = sylar::Socket::CreateTCPSocket();

// 连接服务器
sylar::Address::ptr addr = sylar::Address::LookupAny("127.0.0.1:8080");
sock->connect(addr);

// 发送数据
std::string data = "Hello, World!";
sock->send(data.c_str(), data.size());

// 接收数据
char buffer[1024];
int len = sock->recv(buffer, sizeof(buffer));
```

### 使用ByteArray

```cpp
#include "net/byte_array.hpp"

// 创建ByteArray
sylar::ByteArray::ptr ba(new sylar::ByteArray);

// 写入数据
ba->writeFint32(12345);
ba->writeStringF16("Hello");

// 读取数据
int32_t value = ba->readFint32();
std::string str = ba->readString16();
```

## 协程集成

网络模块与Sylar框架的协程系统深度集成：

1. Socket操作支持协程挂起和恢复
2. Hook机制使系统调用协程化
3. IOManager调度网络事件
4. 超时控制支持协程友好的实现

## 性能优化

1. 使用内存池和对象复用减少内存分配
2. 零拷贝技术优化数据传输
3. 链式缓冲区管理大块数据
4. 读写位置优化减少内存移动

## 线程安全

网络模块的线程安全性如下：

- Socket类：线程不安全，需要外部同步
- Address类：线程安全（只读操作）
- ByteArray类：线程不安全，需要外部同步
- FdManager类：线程安全（内部使用读写锁保护）

## 总结

Sylar框架的网络模块提供了一套完整、高性能、易用的网络编程解决方案。通过面向对象的设计和协程的集成，大大简化了网络编程的复杂性，同时保持了良好的性能和可扩展性。开发者可以基于这些组件快速构建各种网络应用。