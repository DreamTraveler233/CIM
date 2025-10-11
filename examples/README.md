# Sylar框架示例

该目录包含Sylar框架的示例配置和使用方法。

## 目录结构

```
examples/
├── config/           # 示例配置文件
│   ├── log.yaml      # 日志系统配置示例
│   └── test.yaml     # 测试配置示例
└── README.md         # 本文件
```

## 使用方法

1. 将配置文件复制到运行目录的config子目录中：
   ```bash
   cp examples/config/* bin/config/
   ```

2. 运行测试程序时会自动加载这些配置文件。

## 配置文件说明

### log.yaml
日志系统的配置示例，包含不同级别的日志记录器和输出方式。

### test.yaml
测试用配置文件，包含各种数据类型的配置项示例。