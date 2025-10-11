Sylar 日志模块说明文档
1. 概述
Sylar 日志模块是一个功能完整的日志系统，提供了多级别日志记录、灵活的日志格式化、多种输出方式（控制台、文件）以及基于配置文件的动态配置功能。该模块采用模块化设计，具有良好的扩展性和易用性。

2. 模块结构
日志模块由以下几个核心组件构成：

2.1 核心类结构
sylar
├── LogLevel（日志级别）
├── LogEvent（日志事件）
├── LogFormatter（日志格式化器）
├── LogAppender（日志追加器）
│   ├── StdoutLogAppender（控制台输出）
│   └── FileLogAppender（文件输出）
├── Logger（日志记录器）
└── LoggerManager（日志管理器）
2.2 各组件详细说明
LogLevel（日志级别）
定义了6个日志级别，从低到高依次为：

UNKNOW (0)：未知级别
DEBUG (1)：调试信息
INFO (2)：一般信息
WARN (3)：警告信息
ERROR (4)：错误信息
FATAL (5)：致命错误
LogEvent（日志事件）
表示一次日志记录事件，包含以下信息：

文件名
行号
运行时间
线程ID
协程ID
时间戳
日志消息内容
日志级别
所属Logger
LogFormatter（日志格式化器）
负责将日志事件格式化为字符串输出，支持多种格式化项：

%m：消息体
%p：日志级别
%r：运行时间
%c：日志名称
%t：线程ID
%n：换行符
%d：时间（可指定格式，如%d{%Y-%m-%d %H:%M:%S}）
%f：文件名
%l：行号
%T：制表符
%F：协程ID
LogAppender（日志追加器）
负责将格式化后的日志输出到不同目标：

StdoutLogAppender：输出到标准控制台
FileLogAppender：输出到指定文件
Logger（日志记录器）
日志记录的核心类，管理多个LogAppender，负责过滤指定级别的日志并分发给各个Appender。

LoggerManager（日志管理器）
管理所有Logger实例，提供获取Logger的统一入口，支持通过名称获取对应的Logger。

3. 使用说明
3.1 基本使用
cpp
#include "log/log_macros.hpp"

// 获取root logger
auto logger = SYLAR_LOG_ROOT();

// 获取指定名称的logger
auto system_logger = SYLAR_LOG_NAME("system");

// 输出不同级别的日志
SYLAR_LOG_DEBUG(logger) << "debug message";
SYLAR_LOG_INFO(logger) << "info message";
SYLAR_LOG_WARN(logger) << "warn message";
SYLAR_LOG_ERROR(logger) << "error message";
SYLAR_LOG_FATAL(logger) << "fatal message";

// 使用格式化输出
SYLAR_LOG_FMT_INFO(logger, "name: %s, age: %d", "sylar", 18);
3.2 配置文件使用
日志模块支持通过YAML配置文件进行配置，配置文件示例如下：

yaml
logs:
  - name: root
    level: info
    formatter: "%d%T%m%n"
    appenders:
      - type: FileLogAppender
        file: /home/szy/code/sylar/bin/log/root.log
        formatter: "%d%T%m%n"
      - type: StdoutLogAppender
  - name: system
    level: debug
    formatter: "%d%T%m%n"
    appenders:
      - type: FileLogAppender
        file: /home/szy/code/sylar/bin/log/system.log
        formatter: "%d%T%m%m%n"
      - type: StdoutLogAppender
        level: info
3.3 自定义格式
日志格式化支持多种占位符：

%m：日志消息内容
%p：日志级别（DEBUG/INFO/WARN/ERROR/FATAL）
%r：程序启动到现在的毫秒数
%c：日志器名称
%t：线程ID
%n：换行符
%d{format}：时间，format可以是strftime格式
%f：文件名
%l：行号
%T：制表符
%F：协程ID
示例格式："%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%l%T%m%n"

4. 高级特性
4.1 多输出目标
支持同时输出到控制台和文件，也可以只输出到其中一个。

4.2 线程安全
整个日志系统设计为线程安全，可以在多线程环境中安全使用。

4.3 动态配置
支持运行时通过配置文件动态修改日志级别、输出格式和输出目标。

4.4 单例模式
LoggerManager采用单例模式，确保全局只有一个日志管理器实例。

5. 最佳实践
为不同模块创建不同的Logger，便于日志分类管理
合理设置日志级别，在生产环境中避免输出过多DEBUG信息
使用配置文件管理日志配置，便于运行时调整
合理设计日志格式，包含足够信息便于问题排查
注意日志文件大小控制，避免占用过多磁盘空间
6. 性能考虑
通过宏定义在编译期进行日志级别判断，避免不必要的日志处理
使用智能指针管理对象生命周期
格式化操作仅在需要输出时进行
支持多Appender并行输出
7. 扩展性
可以自定义LogAppender实现其他输出方式（如网络传输）
可以自定义FormatItem实现特殊格式化需求
可以通过继承Logger实现特殊日志处理逻辑
以上是Sylar日志模块的完整说明文档，通过合理使用该模块，可以为应用程序提供强大而灵活的日志功能。