# 日志模块API详解

## 概述

日志模块是一个功能完整、线程安全、高性能的日志系统。该模块提供了灵活的日志记录功能，支持多种输出方式（控制台、文件）、日志级别控制、自定义格式化、日志轮转等功能，使用简单，只需要包含头文件`#include "macro.hpp"`即可使用。

日志模块采用分层设计，主要包括以下几个核心组件：

1. [LogLevel（日志等级）](#1-loglevel---日志等级) - 定义日志的严重等级
2. [LogEvent（日志事件）](#2-logevent---日志事件) - 封装单条日志的所有信息
3. [LogFormatter（日志格式化器）](#3-logformatter---日志格式化器) - 控制日志的输出格式
4. [LogAppender（日志输出器）](#4-logappender---日志输出器) - 控制日志的输出目标
5. [Logger（日志器）](#5-logger---日志器) - 管理日志的记录和分发
6. [LoggerManager（日志管理器）](#6-loggermanager---日志管理器) - 管理所有的日志器实例
7. [LogFile（日志文件）](#7-logfile---日志文件) - 管理日志文件的读写和轮转
8. [LogFileManager（日志文件管理器）](#8-logfilemanager---日志文件管理器) - 管理所有日志文件实例

## 核心组件详解

### 1. LogLevel - 日志等级

日志等级从低到高依次为：DEBUG < INFO < WARN < ERROR < FATAL

#### 枚举值

```cpp
enum class Level
{
    UNKNOWN = 0, // 未知等级
    DEBUG = 1,   // 调试信息等级
    INFO = 2,    // 普通信息等级
    WARN = 3,    // 警告信息等级
    ERROR = 4,   // 错误信息等级
    FATAL = 5    // 致命错误等级
};
```

#### 主要方法

- `static const char *ToString(Level level)` - 将日志等级转换为字符串
- `static Level FromString(const std::string &str)` - 将字符串转换为日志等级

### 2. LogEvent - 日志事件

LogEvent封装了一次日志记录的所有相关信息，包括源文件名、行号、线程ID、协程ID、时间戳等元信息，以及日志消息内容。

#### 主要方法

- `LogEvent::ptr getEvent() const` - 获取日志事件
- `std::stringstream &getSS()` - 获取消息内容流
- `const char *getFileName() const` - 获取源文件名
- `int32_t getLine() const` - 获取行号
- `uint32_t getThreadId() const` - 获取线程ID
- `const std::string &getThreadName() const` - 获取线程名称
- `uint32_t getCoroutineId() const` - 获取协程ID
- `uint64_t getTime() const` - 获取时间戳
- `std::string getMessage() const` - 获取日志消息内容
- `std::shared_ptr<Logger> getLogger() const` - 获取关联的日志器
- `Level getLevel() const` - 获取日志等级
- `void format(const char *fmt, ...)t` - 格式化写入日志内容
- `void format(const char *fmt, va_list al)` - 格式化写入日志内容

### 3. LogFormatter - 日志格式化器

LogFormatter负责将日志事件按照指定的格式模式转换为字符串输出。

#### 支持的格式化项

| 格式符 | 含义 |
|--------|------|
| %m     | 日志消息内容 |
| %p     | 日志级别 |
| %r     | 程序启动到现在的耗时(毫秒) |
| %c     | 日志器名称 |
| %t     | 线程ID |
| %N     | 线程名称 |
| %F     | 协程ID |
| %d     | 时间 |
| %f     | 源文件名 |
| %l     | 行号 |
| %T     | 制表符 |
| %n     | 换行符 |

#### 主要方法

- `LogFormatter(const std::string &pattern)` - 构造函数，指定格式模式
- `std::string format(std::shared_ptr<LogEvent> event)` - 格式化日志事件

### 4. LogAppender - 日志输出器

LogAppender定义日志的输出目标和格式化方式，是日志系统的核心组件之一。

#### 派生类

- **StdoutLogAppender** - 标准输出日志追加器，将日志输出到标准输出流
- **FileLogAppender** - 文件日志追加器，将日志输出到指定文件

#### 主要方法

- `virtual void log(LogEvent::ptr event) = 0` - 写入日志事件
- `virtual std::string toYamlString() = 0` - 将追加器配置转换为YAML字符串
- `void setFormatter(LogFormatter::ptr formatter)` - 设置日志格式器
- `void setLevel(Level level)` - 设置日志级别

### 5. Logger - 日志器

Logger是日志系统的中枢组件，负责管理日志的输出目标和日志级别的控制。

#### 主要方法

- `void log(Level level, LogEvent::ptr event)` - 记录日志
- `void debug/info/warn/error/fatal(LogEvent::ptr event)` - 记录特定级别日志
- `void addAppender(LogAppender::ptr appender)` - 添加日志输出器
- `void delAppender(LogAppender::ptr appender)` - 删除日志输出器
- `void clearAppender()` - 清空所有日志输出器
- `void setLevel(Level level)` - 设置日志级别
- `void setFormatter(LogFormatter::ptr val)` - 设置日志格式器
- `std::string toYamlString()` - 将日志器配置转换为YAML格式字符串

### 6. LoggerManager - 日志管理器

LoggerManager负责管理系统中所有的日志记录器(Logger)实例。

#### 主要方法

- `std::shared_ptr<Logger> getLogger(const std::string &name)` - 获取指定名称的日志记录器
- `std::shared_ptr<Logger> getRoot() const` - 获取根日志记录器
- `std::string toYamlString()` - 将日志管理器配置转换为YAML字符串

### 7. LogFile - 日志文件

LogFile负责管理日志文件的读写和轮转。

#### 枚举值
```cpp
enum class RotateType
{
    NONE,   ///< 不轮转
    MINUTE, ///< 按分钟轮转
    HOUR,   ///< 按小时轮转
    DAY     ///< 按天轮转
};
```

#### 主要方法

- `bool openFile()` - 打开文件
- `size_t writeLog(const std::string &logMsg)` - 写入文件
- `void rotate(const std::string &newFilePath)` - 日志文件轮转（切换）

### 8. LogFileManager - 日志文件管理器

LogFileManager是一个单例类，负责日志文件（LogFile）管理类，负责日志文件对象的创建、轮转和管理。

#### 主要方法

- `LogFile::ptr getLogFile(const std::string &fileName)` - 获取指定名称日志文件的智能指针
- `void init()` - 初始化日志管理器
- `void onCheck()` - 检查日志文件是否需要轮转（按分钟、小时、天），并执行轮转操作
- `void rotateMinute(const LogFile::ptr &file)` - 按分钟进行日志轮转
- `void rotateHours(const LogFile::ptr &file)` - 按小时进行日志轮转
- `void rotateDays(const LogFile::ptr &file)` - 按天进行日志轮转

## 使用指南

### 基本使用

```cpp
#include "macro.hpp" // 头文件 macro.hpp 包含了常用日志输出宏

// 获取系统根日志器
/*
    根日志器默认配置: 
        name: root
        level: DEBUG
        formatter: %d%T%N%T%t%T%F%T[%p]%T[%c]%T<%f:%l>%T%m%n
        appenders: StdoutLogAppender

*/
auto g_logger = SYLAR_LOG_ROOT();

// 使用基本日志宏
SYLAR_LOG_DEBUG(g_logger) << "这是一条DEBUG级别的日志";
SYLAR_LOG_INFO(g_logger) << "这是一条INFO级别的日志";
SYLAR_LOG_WARN(g_logger) << "这是一条WARN级别的日志";

// 使用格式化日志宏
SYLAR_LOG_FMT_INFO(g_logger, "用户 %s 登录成功，IP地址: %s", username.c_str(), ip.c_str());
SYLAR_LOG_FMT_ERROR(g_logger, "数据库连接失败，错误码: %d, 错误信息: %s", err_code, err_msg.c_str());
SYLAR_LOG_FMT_DEBUG(g_logger, "处理完成，耗时: %ld 毫秒，处理数据量: %d 条", time_used, data_count);
```

### 自定义日志器



```cpp
#include "macro.hpp"

/*
 自定义日志器的配置采用热加载的模式，默认为系统根日志的配置
*/

// 创建自定义日志器，并设置全局配置
auto g_logger = SYLAR_LOG_NAME("my_logger");
g_logger->setFormatter(std::make_shared<sylar::LogFormatter>("%d%T%t%T%m%n"));
g_logger->setLevel(Level::INFO);

// 添加文件输出器，并设置单独的配置
auto file_appender = std::make_shared<sylar::FileLogAppender>("my_logger.log");
file_appender->setFormatter(std::make_shared<sylar::LogFormatter>("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%l%T%m%n"));
g_logger->setLevel(Level::DEBUG);
g_logger->addAppender(file_appender);

// 添加控制台输出器，没有单独配置，则采用全局配置
auto stdout_appender = std::make_shared<sylar::StdoutLogAppender>();
g_logger->addAppender(stdout_appender);

// 记录日志
SYLAR_LOG_INFO(g_logger) << "这是一条自定义日志";
```

### 完整示例

```cpp
#include "macro.hpp"

void setupLogger() {
    // 获取根日志器
    auto root_logger = SYLAR_LOG_ROOT();
    
    // 创建并配置控制台输出器
    auto console_appender = std::make_shared<sylar::StdoutLogAppender>();
    console_appender->setLevel(sylar::Level::INFO);
    console_appender->setFormatter(
        std::make_shared<sylar::LogFormatter>("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T[%p]%T[%c]%T%f:%l%T%m%n")
    );
    
    // 创建并配置文件输出器
    auto file_appender = std::make_shared<sylar::FileLogAppender>("logs/app.log");
    file_appender->setLevel(sylar::Level::DEBUG);
    file_appender->setFormatter(
        std::make_shared<sylar::LogFormatter>("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T[%p]%T[%c]%T%f:%l%T%m%n")
    );
    
    // 设置日志轮转类型
    file_appender->getLogFile()->setRotateType(sylar::RotateType::HOUR);
    
    // 添加输出器到日志器
    root_logger->addAppender(console_appender);
    root_logger->addAppender(file_appender);
    
    // 设置日志器级别
    root_logger->setLevel(sylar::Level::DEBUG);
}

int main() {
    // 初始化日志系统
    setupLogger();
    
    // 获取日志器
    auto logger = SYLAR_LOG_ROOT();
    
    // 记录不同级别的日志
    SYLAR_LOG_DEBUG(logger) << "Debug message";
    SYLAR_LOG_INFO(logger) << "Info message";
    SYLAR_LOG_WARN(logger) << "Warning message";
    SYLAR_LOG_ERROR(logger) << "Error message";
    
    return 0;
}
```

## 高级功能

### 配置文件支持

通过配置文件设置日志器：

```cpp
// 创建日志器
auto g_logger = SYLAR_LOG_ROOT();

// 从配置文件加载配置
YAML::Node config = YAML::LoadFile("log.yaml");
sylar::Config::LoadFromYaml(config);

SYLAR_LOG_INFO(g_logger) << "通过配置文件设置日志器";
```

日志模块支持通过YAML配置文件进行配置：

```yaml
logs:
  - name: root
    level: Info
    formatter: "%d%T%m%n"
    appenders:
      - type: FileLogAppender
        path: /home/szy/code/sylar/bin/log/root.log
        formatter: "%d%T%t%T%m%n"
        rotate_type: minute
      - type: StdoutLogAppender
        formatter: "%d%T<%f:%l>%m%n"
        
  - name: system
    level: debug
    formatter: "%d%T%m%n"
    appenders:
      - type: FileLogAppender
        path: /home/szy/code/sylar/bin/log/system.log
        level: DEBUG
        formatter: "%d%T<%f:%l>%m%n"
        rotate_type: Hour
      - type: StdoutLogAppender
```

### 日志轮转功能

Sylar日志模块支持自动日志轮转功能，可以按照时间单位自动分割日志文件，避免单个日志文件过大。

#### 轮转类型

- **NONE** - 不轮转
- **MINUTE** - 按分钟轮转
- **HOUR** - 按小时轮转
- **DAY** - 按天轮转

#### 轮转文件命名规则

- 分钟轮转文件名：原文件名_YYYY-MM-DDTHHMM.扩展名
- 小时轮转文件名：原文件名_YYYY-MM-DDTHH.扩展名
- 天轮转文件名：原文件名_YYYY-MM-DD.扩展名

#### 使用方式

```cpp
// 设置轮转类型
auto file_appender = std::make_shared<sylar::FileLogAppender>("app.log");
file_appender->getLogFile()->setRotateType(sylar::RotateType::DAY);

// 或者通过配置文件设置 rotate_type 字段
```

## 系统特性

### 线程安全性

日志模块的所有组件都是线程安全的，可以在多线程环境中安全使用。内部使用互斥锁保护共享资源，确保并发访问的安全性。

### 性能特点

1. **高效** - 采用流式日志记录方式，减少字符串拼接开销
2. **灵活** - 支持多种输出目标和自定义格式
3. **可扩展** - 易于扩展新的输出器和格式化项
4. **线程安全** - 支持多线程并发访问
5. **配置化** - 支持YAML配置文件动态配置
6. **自动轮转** - 支持按时间自动轮转日志文件