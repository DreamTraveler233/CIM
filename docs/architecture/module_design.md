# Sylar框架模块设计

## 1. 模块组织结构

Sylar框架按照功能职责划分为以下模块：

### 1.1 core（核心模块）
包含最基础、被其他模块广泛依赖的组件：
- `macro.hpp` - 宏定义
- `noncopyable.hpp` - 不可复制基类
- `singleton.hpp` - 单例模板
- `util.hpp` - 通用工具函数
- `lock.hpp` - 锁相关实现
- `semaphore.hpp` - 信号量实现
- `thread.hpp` - 线程相关实现
- `timer.hpp` - 定时器实现

### 1.2 runtime（运行时模块）
包含协程和调度相关的组件：
- `coroutine.hpp` - 协程实现
- `scheduler.hpp` - 调度器实现
- `iomanager.hpp` - IO管理器实现

### 1.3 system（系统接口模块）
包含与系统调用和文件描述符管理相关的组件：
- `hook.hpp` - 系统调用拦截
- `fd_manager.hpp` - 文件描述符管理

### 1.4 net（网络模块）
专门处理网络地址和更高层的网络功能：
- `address.hpp` - 网络地址处理
- `endian.hpp` - 字节序处理
- `socket.hpp` - Socket封装

### 1.5 config（配置模块）
独立的配置管理模块：
- `config.hpp` - 配置管理核心
- `config_var.hpp` - 配置变量实现
- `config_var_base.hpp` - 配置变量基类
- `lexical_cast.hpp` - 类型转换实现

### 1.6 log（日志模块）
独立的日志系统模块：
- `log_level.hpp` - 日志级别定义
- `log_event.hpp` - 日志事件处理
- `log_formatter.hpp` - 日志格式化
- `log_appender.hpp` - 日志输出器
- `logger.hpp` - 日志记录器
- `logger_manager.hpp` - 日志管理器

## 2. 模块依赖原则

模块间依赖应呈现清晰的层次结构：
```
config, log -> core -> runtime -> system -> net
```

即：
- config和log模块依赖core模块
- runtime模块依赖core模块
- system模块依赖runtime模块
- net模块依赖system模块

## 3. 设计原则

1. **高内聚低耦合** - 每个模块职责分明，功能相关性强
2. **单一职责原则** - 每个文件只包含单一功能或相关功能的实现
3. **依赖方向明确** - 模块间依赖关系清晰，避免循环依赖
4. **功能内聚性优先** - 同一模块内的组件具有高度相关的功能职责