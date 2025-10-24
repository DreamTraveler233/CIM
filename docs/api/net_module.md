# CIM框架网络模块文档

## 概述

CIM框架的网络模块提供了一套完整的网络编程解决方案，包括地址封装、Socket操作、流处理、TCP服务器等核心组件。该模块设计目标是提供高性能、易用性和可扩展性，支持IPv4、IPv6和Unix域套接字，并与框架的协程系统无缝集成。

### 整体架构层次

网络模块采用分层设计思想，每一层都有明确的职责和接口：

```
+--------------------------------------------------+
|               应用层 (TCP Server)                 |
+--------------------------------------------------+
|               流层 (Stream/SocketStream)          |
+--------------------------------------------------+
|           Socket层 (Socket/SSLSocket)            |
+--------------------------------------------------+
|         地址层 (Address/IPAddress/...)           |
+--------------------------------------------------+
|     系统接口层 (Hook/FdManager/协程调度)          |
+--------------------------------------------------+
```

### 核心组件详解

#### Address地址模块
这是网络模块的基础，用于表示各种类型的网络地址：
- Address基类：抽象了所有地址类型的公共接口，提供地址创建、比较、字符串转换等基本功能
- IPAddress类：继承自Address，专门处理IP地址，支持IPv4和IPv6地址，提供广播地址、网络地址、子网掩码等网络计算功能
- UnixAddress类：处理Unix域套接字地址
- UnknownAddress类：处理未知类型的地址

关键功能包括主机名解析（支持域名到IP的转换）、本地网卡接口地址获取、地址比较和字符串表示。

#### Socket套接字模块
Socket类是对系统套接字的高级封装，提供了完整的面向对象接口：
- 支持TCP套接字（SOCK_STREAM）和UDP套接字（SOCK_DGRAM）
- 支持IPv4 (AF_INET)、IPv6 (AF_INET6)和Unix域套接字 (AF_UNIX)
- 核心功能包括连接管理（connect, bind, listen, accept）、数据传输（send, recv及其变种）、超时控制和SSL/TLS加密通信支持（SSLSocket子类）

设计特点包括使用智能指针管理内存、提供工厂方法创建不同类型Socket实例、支持协程友好的异步I/O操作。

#### Stream流模块
Stream抽象了数据的读写操作，提供了统一的I/O接口：
- Stream基类定义了基本的读写接口（read/write）和固定长度数据读写的便捷方法
- SocketStream类继承自Stream，将Socket包装成流的形式，实现了Stream接口与Socket的对接

#### TCP服务器模块
这是一个高级封装，用于快速构建TCP服务器：
- 支持多地址绑定
- 集成IOManager进行协程调度
- 提供SSL支持
- 支持配置化管理
- 实现了完整的连接处理流程

#### 底层支撑模块
- Hook模块对系统调用进行hook，使其对协程友好，实现非阻塞I/O操作的自动挂起和恢复
- 文件描述符管理器统一管理系统中所有的文件描述符，管理文件描述符的状态、超时等属性
- 字节数组提供高效的数据序列化和反序列化，支持多种数据类型的读写

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

Address基类还提供了一些静态方法用于地址解析和创建：
- `Lookup`系列方法支持通过主机名（域名或IP地址）解析出地址列表
- `GetInterfaceAddresses`方法支持获取本地网卡接口地址列表

### IPAddress类

IPAddress继承自Address，专门用于表示IP地址（IPv4和IPv6），提供了广播地址、网络地址、子网掩码等相关操作：

- `broadcastAddress` - 获取广播地址
- `networkAddress` - 获取网络地址
- `subnetMask` - 获取子网掩码地址
- `getPort` - 获取端口号
- `setPort` - 设置端口号
- `Create` - 静态工厂方法，通过IP地址字符串创建IPAddress对象

IPAddress支持端口号的管理，这对于网络编程非常重要。端口号在网络通信中用于标识不同的服务或应用程序。

### IPv4Address类

IPv4Address是IPv4地址的具体实现，支持IPv4地址的各种操作。IPv4地址使用32位表示，通常以点分十进制格式表示（如192.168.1.1）。

### IPv6Address类

IPv6Address是IPv6地址的具体实现，支持IPv6地址的各种操作。IPv6地址使用128位表示，通常以冒号分隔的十六进制格式表示（如2001:0db8:85a3:0000:0000:8a2e:0370:7334）。

### UnixAddress类

UnixAddress用于表示Unix域套接字地址。Unix域套接字是一种在同一台主机上进行进程间通信的方式，使用文件系统路径作为地址。

### UnknownAddress类

UnknownAddress用于表示未知类型的地址。当系统无法识别特定类型的地址时，会使用此类进行封装。

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

Socket类提供了丰富的工厂方法来创建不同类型的Socket实例，包括：
- 针对特定地址类型创建TCP或UDP Socket
- 创建IPv4或IPv6的TCP或UDP Socket
- 创建Unix域的TCP或UDP Socket

这些工厂方法简化了Socket的创建过程，开发者不需要手动指定协议族、类型等参数。

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

Socket类提供了完整的网络编程接口：
- 连接管理：支持客户端的connect操作和服务端的bind/listen/accept操作
- 数据传输：提供面向连接的send/recv和面向无连接的sendTo/recvFrom方法
- 状态控制：支持超时设置、Socket选项配置和连接状态查询

#### 超时和选项控制
Socket类支持灵活的超时控制和选项配置：
- 发送和接收超时设置，避免操作无限期阻塞
- 丰富的Socket选项操作接口，支持获取和设置各种Socket参数
- 模板化选项操作方法，简化不同类型参数的设置

#### SSL支持
SSLSocket继承自Socket，提供基于SSL/TLS的安全网络通信功能。

SSLSocket扩展了基本的Socket功能，添加了SSL/TLS加密支持：
- 支持安全的网络通信
- 提供证书加载和验证功能
- 实现加密的数据传输

## Stream模块

Stream模块定义了流操作的抽象接口，为数据的读写提供统一的接口。

### Stream类

Stream类定义了流的基本操作接口：

- `read` - 读取数据
- `write` - 写入数据
- `close` - 关闭流
- 固定长度读写的辅助方法

Stream抽象类为各种流式数据传输提供了统一接口：
- 基本读写操作：支持从流中读取数据和向流中写入数据
- 固定长度操作：提供readFixSize和writeFixSize方法，确保读写指定长度的数据
- ByteArray支持：支持直接与ByteArray对象进行数据交互
- 状态管理：提供close方法用于关闭流

Stream的设计使得无论是文件、网络Socket还是其他类型的数据源，都可以通过统一的接口进行操作。

### SocketStream类

SocketStream是Stream的具体实现，基于Socket提供流式数据传输功能。

SocketStream将Socket对象包装成流的形式，使得Socket可以像普通流一样使用：
- 继承自Stream类，实现所有抽象方法
- 内部封装Socket对象，通过Socket进行实际的数据传输
- 提供与Socket直接交互的方法，如获取远程地址、本地地址等
- 支持连接状态检查

## TCPServer模块

TCPServer模块提供了TCP服务器的封装实现。

### TcpServer类

TcpServer类提供了TCP服务器的核心功能：

- 多地址绑定支持
- SSL支持
- 协程调度器集成
- 客户端连接处理
- 服务器启停控制

TcpServer是构建TCP服务器应用的核心类：
- 构造时可指定工作协程调度器，支持不同的并发处理策略
- 支持绑定多个地址，提高服务的可用性
- 集成SSL支持，可提供安全的网络通信
- 通过handleClient虚函数支持自定义客户端处理逻辑
- 提供完整的服务器生命周期管理（start/stop）

TcpServer通过startAccept方法启动连接接收过程，当有新连接到达时会自动调用handleClient方法处理客户端连接。

### TcpServerConf结构

TcpServerConf用于配置TCP服务器的各种参数：

- 地址列表
- Keepalive设置
- 超时设置
- SSL配置
- 工作协程调度器配置

TcpServerConf结构提供了一种配置化的服务器管理方式：
- 支持YAML等配置文件格式
- 可动态调整服务器配置
- 支持多种工作模式配置

## Hook模块

Hook模块实现了对系统调用的拦截，使其协程化，避免阻塞整个线程。

### 主要功能

- 检查和设置钩子功能启用状态
- 拦截常见的系统调用（sleep、socket相关、IO操作等）
- 使阻塞操作变为协程挂起和恢复

Hook机制是CIM框架协程系统的重要组成部分：
- 对sleep、usleep、nanosleep等睡眠函数进行hook，使其能够自动挂起和恢复协程
- 对socket相关的系统调用（如connect、accept、read、write等）进行hook，实现非阻塞操作
- 通过IOManager与协程调度系统集成，当I/O操作无法立即完成时自动挂起协程

Hook模块使得传统的阻塞式系统调用在协程环境中变得非阻塞，大大简化了异步编程的复杂性。

## FdManager模块

FdManager模块提供了文件描述符的统一管理机制。

### FdCtx类

FdCtx类管理单个文件描述符的上下文信息：

- 初始化状态
- Socket类型标识
- 阻塞模式设置
- 超时时间管理

FdCtx跟踪和管理每个文件描述符的重要属性：
- 记录文件描述符是否已初始化和是否已关闭
- 区分Socket类型和普通文件类型
- 管理用户设置的非阻塞模式和系统设置的非阻塞模式
- 维护发送和接收超时时间

### FdManager类

FdManager类是全局的文件描述符管理器：

- 线程安全的文件描述符上下文获取和删除
- 单例模式实现

FdManager通过单例模式提供全局访问点：
- 使用读写锁保证线程安全
- 支持自动创建文件描述符上下文
- 提供高效的文件描述符上下文查找

### FileDescriptor类

FileDescriptor类提供了RAII风格的文件描述符封装，自动管理文件描述符的生命周期。

FileDescriptor遵循RAII原则，确保文件描述符得到正确管理：
- 构造时可以接受文件描述符
- 析构时自动关闭文件描述符
- 支持移动语义，便于对象传递
- 提供reset和release方法，支持灵活的生命周期管理

## ByteArray模块

ByteArray模块提供了对可变长度字节数据流的处理功能。

### 主要特性

- 链表结构管理内存块
- 支持多种数据类型的读写操作
- 固定长度和变长编码支持
- 大小端字节序转换
- 文件读写支持
- 网络传输优化（iovec支持）

ByteArray使用链表结构管理内存块，具有以下优势：
- 避免频繁的内存重新分配和拷贝
- 支持动态扩展容量
- 提供高效的随机访问和顺序访问

### 核心功能

- 基本数据类型读写（整数、浮点数、字符串）
- 固定长度和变长编码整数读写
- 字符串读写（多种长度编码方式）
- 位置控制和容量管理
- 数据序列化和反序列化

ByteArray支持多种数据类型的读写操作：
- 整数类型：支持8位、16位、32位、64位有符号和无符号整数
- 浮点数类型：支持float和double类型
- 字符串类型：支持多种长度编码方式（固定长度、变长编码等）
- 提供getReadSize等方法查询可读数据大小

## 使用示例

### 创建TCP服务器

```
#include "net/tcp_server.hpp"
#include "net/socket.hpp"

// 创建TCP服务器
CIM::TcpServer::ptr server(new CIM::TcpServer);

// 创建并绑定地址
CIM::Address::ptr addr = CIM::Address::LookupAny("0.0.0.0:8080");
server->bind(addr);

// 启动服务器
server->start();
```

在实际应用中，通常需要继承TcpServer类并重写handleClient方法来处理客户端连接：

```
class EchoServer : public CIM::TcpServer {
public:
    void handleClient(CIM::Socket::ptr client) override {
        // 处理客户端连接
        char buffer[1024];
        while(true) {
            int n = client->recv(buffer, sizeof(buffer));
            if(n <= 0) {
                break;
            }
            client->send(buffer, n);
        }
    }
};
```

### 创建TCP客户端

```
#include "net/socket.hpp"

// 创建TCP Socket
CIM::Socket::ptr sock = CIM::Socket::CreateTCPSocket();

// 连接服务器
CIM::Address::ptr addr = CIM::Address::LookupAny("127.0.0.1:8080");
sock->connect(addr);

// 发送数据
std::string data = "Hello, World!";
sock->send(data.c_str(), data.size());

// 接收数据
char buffer[1024];
int len = sock->recv(buffer, sizeof(buffer));
```

在实际应用中，可能需要添加错误处理和超时控制：

```
CIM::Socket::ptr sock = CIM::Socket::CreateTCPSocket();
sock->setRecvTimeout(5000); // 设置接收超时为5秒
sock->setSendTimeout(5000); // 设置发送超时为5秒

if(!sock->connect(addr, 3000)) { // 连接超时3秒
    std::cout << "连接失败" << std::endl;
    return;
}
```

### 使用ByteArray

```
#include "net/byte_array.hpp"

// 创建ByteArray
CIM::ByteArray::ptr ba(new CIM::ByteArray);

// 写入数据
ba->writeFint32(12345);
ba->writeStringF16("Hello");

// 读取数据
int32_t value = ba->readFint32();
std::string str = ba->readString16();
```

ByteArray在协议解析和序列化中非常有用：

```
// 序列化复杂数据结构
struct Person {
    std::string name;
    int age;
    double salary;
};

void serializePerson(CIM::ByteArray::ptr ba, const Person& p) {
    ba->writeStringF16(p.name);
    ba->writeFint32(p.age);
    ba->writeFdouble(p.salary);
}

Person deserializePerson(CIM::ByteArray::ptr ba) {
    Person p;
    p.name = ba->readStringF16();
    p.age = ba->readFint32();
    p.salary = ba->readFdouble();
    return p;
}
```

### 使用SocketStream

```
#include "net/socket_stream.hpp"

// 创建Socket
CIM::Socket::ptr sock = CIM::Socket::CreateTCPSocket();
sock->connect(CIM::Address::LookupAny("127.0.0.1:8080"));

// 创建SocketStream
CIM::SocketStream::ptr stream(new CIM::SocketStream(sock));

// 使用流接口进行操作
char buffer[1024];
int n = stream->read(buffer, sizeof(buffer));
if(n > 0) {
    stream->write(buffer, n);
}
```

### 地址解析和使用

```
#include "net/address.hpp"

// 解析域名
std::vector<CIM::Address::ptr> results;
CIM::Address::Lookup(results, "www.google.com", AF_INET);

// 创建IP地址
CIM::IPAddress::ptr addr = CIM::IPAddress::Create("192.168.1.1", 8080);

// 获取本地网络接口地址
std::multimap<std::string, std::pair<CIM::Address::ptr, uint32_t>> interfaces;
CIM::Address::GetInterfaceAddresses(interfaces);
```

## 协程集成

网络模块与CIM框架的协程系统深度集成：

1. Socket操作支持协程挂起和恢复
2. Hook机制使系统调用协程化
3. IOManager调度网络事件
4. 超时控制支持协程友好的实现

协程集成是CIM网络模块的核心特性之一：
- 当网络I/O操作无法立即完成时，协程会自动挂起，不会阻塞线程
- 通过IOManager监听文件描述符事件，当条件满足时自动恢复协程执行
- 实现了同步编程模型下的异步性能，大大简化了异步网络编程的复杂性
- 支持超时控制，避免协程无限期挂起

这种设计使得开发者可以用同步的方式编写代码，但获得异步的性能表现。

## 性能优化

1. 使用内存池和对象复用减少内存分配
2. 零拷贝技术优化数据传输
3. 链式缓冲区管理大块数据
4. 读写位置优化减少内存移动

网络模块采用了多种性能优化技术：
- 内存池：对于频繁创建和销毁的对象（如协程、网络数据包等）使用内存池管理，减少内存分配开销
- 零拷贝：在网络数据传输过程中尽可能避免数据拷贝，提高传输效率
- 链式缓冲区：ByteArray使用链表结构管理内存块，避免频繁的内存重新分配
- 读写位置优化：通过维护读写位置指针，减少数据移动操作

## 线程安全

网络模块的线程安全性如下：

- Socket类：线程不安全，需要外部同步
- Address类：线程安全（只读操作）
- ByteArray类：线程不安全，需要外部同步
- FdManager类：线程安全（内部使用读写锁保护）

在多线程环境中使用网络模块时需要注意：
- Socket和ByteArray对象应避免在多个线程间共享，或使用外部同步机制保护
- Address对象是只读的，可以在多个线程间安全共享
- FdManager使用读写锁保护内部数据结构，支持多线程并发访问

## 总结

CIM框架的网络模块提供了一套完整、高性能、易用的网络编程解决方案。通过面向对象的设计和协程的集成，大大简化了网络编程的复杂性，同时保持了良好的性能和可扩展性。开发者可以基于这些组件快速构建各种网络应用。

网络模块的主要优势包括：
- 完整的网络编程抽象，涵盖从地址解析到数据传输的各个环节
- 与协程系统的深度集成，简化异步编程
- 高性能设计，采用多种优化技术
- 易用的API设计，提供丰富的工厂方法和高层封装
- 良好的扩展性，支持SSL、IPv6等高级特性
