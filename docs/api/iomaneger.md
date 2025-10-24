# IO模块架构详解

## 整体架构关系
```
                           IOManager(epoll事件循环)
              ________________|________________
              |                                |
              v                                v  
          Scheduler (协程调度)            TimerManager(定时器管理)
    __________|__________                      |
    |                   |                      v
    v                   v                    Timer(定时器)
Coroutine (协程)     Thread(线程)
```

## 框架整体设计思想

整体框架采用分层设计，底层基于协程实现异步非阻塞IO操作。IOManager作为核心组件，整合了epoll事件循环、协程调度和定时器管理三大功能，为上层网络服务提供高性能的异步IO支持。

### 协程驱动的异步编程模型

传统的网络编程通常采用线程池模型或Reactor模型，但这些模型在面对高并发场景时往往面临线程切换开销大、编程复杂等问题。sylar框架采用了协程技术，将异步操作同步化，既保证了性能又简化了编程模型。

当协程执行IO操作时，如果发现IO未就绪（例如读取数据但缓冲区为空），协程会主动挂起并让出执行权给调度器，调度器继续执行其他协程。当IO就绪时，IOManager会收到epoll通知，并重新调度之前挂起的协程继续执行，这个过程对开发者是透明的。

### 分层架构优势

1. **IOManager层**：负责底层epoll事件监听和处理，统一管理所有文件描述符的IO事件。
2. **Scheduler层**：负责协程调度，在工作线程中执行具体的协程任务。
3. **应用层**：如TcpServer、HttpServer等网络服务器，只需关注业务逻辑而无需关心底层IO细节。

IOManager通过多重继承实现了三种核心功能的融合：
1. Scheduler角色 - 负责协程调度和任务执行
2. IO事件管理者角色 - 负责epoll事件监听和处理
3. TimerManager角色 - 负责定时任务管理

## 核心组件详解

### 1. Scheduler角色 - 任务执行者和调度者

作为Scheduler的子类，IOManager继承了协程调度的能力：
- 管理协程队列，维护待执行的任务列表
- 控制工作线程池，协调多个线程执行任务
- 提供基础的调度接口，如schedule()方法用于添加任务

### 2. IO事件管理者角色 - 事件监听者和触发者

这是IOManager最核心的功能：
- 通过epoll监听文件描述符的读写事件（使用边缘触发ET模式）
- 当IO事件就绪时，自动将相关协程或回调函数加入调度队列
- 实现了事件驱动的任务生成机制

### 3. TimerManager角色 - 定时任务管理者

通过继承TimerManager，IOManager还具备了定时任务管理能力：
- 管理各种定时器任务
- 在适当时机触发定时任务执行
- 与IO事件处理无缝集成

## 核心数据结构

### FdContext - 文件描述符上下文

FdContext用于管理特定文件描述符上的事件信息，包括：
- 文件描述符(fd)
- 读写事件上下文(read/write)
- 当前监听的事件类型(events)
- 保护该上下文的互斥锁(mutex)

### EventContext - 事件上下文

EventContext存储特定事件（读或写）的相关信息：
- 关联的调度器(scheduler)
- 绑定到事件的协程对象(coroutine)
- 事件触发时执行的回调函数(cb)

## 使用示例

在网络编程中，开发者可以通过如下方式使用IOManager：

```cpp
// 创建IOManager实例
IOManager iom(2); // 2个工作线程

// 添加一个定时任务
iom.addTimer(1000, [](){
    std::cout << "Timer executed" << std::endl;
});

// 监听socket读事件
iom.addEvent(socket_fd, IOManager::READ, [socket_fd](){
    // 处理读事件
    char buffer[1024];
    int n = read(socket_fd, buffer, sizeof(buffer));
    // 处理数据...
});
```