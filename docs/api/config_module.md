# 配置模块API详解

## 概述

配置模块是一个功能完整、线程安全的配置管理系统。该模块提供了灵活的配置项管理功能，支持从YAML文件加载配置，支持任意数据类型的配置项，提供配置变更监听回调机制。使用简单，只需要包含头文件`#include "config.hpp"`即可使用。

配置模块采用分层设计，主要包括以下几个核心组件：

1. [Config（配置管理器）](#1-config配置管理器) - 配置系统的全局管理类
2. [ConfigVariableBase（配置变量基类）](#2-configvariablebase配置变量基类) - 所有配置变量的基类
3. [ConfigVar（配置变量模板类）](#3-configvar配置变量模板类) - 具体类型的配置变量实现
4. [LexicalCast（词法转换器）](#4-lexicalcast词法转换器) - 配置值与字符串之间的转换工具

## 核心组件详解

### 1. Config - 配置管理器

Config是配置系统的核心管理类，提供了全局的配置项管理功能。

#### 主要功能

1. 配置项的注册和查找
2. 从YAML文件加载配置
3. 线程安全的配置访问
4. 配置项遍历功能

#### 主要方法

- `template <class T> static typename ConfigVar<T>::ptr Lookup(const std::string &name, const T &default_value, const std::string &description = "")` - 查找或创建配置项
- `template <class T> static typename ConfigVar<T>::ptr Lookup(const std::string &name)` - 根据名称查找配置项
- `static ConfigVariableBase::ptr LookupBase(const std::string &name)` - 根据名称查找配置项（基类版本）
- `static void LoadFromYaml(const YAML::Node &root)` - 从YAML配置文件中加载配置项
- `static void Visit(std::function<void(ConfigVariableBase::ptr)> cb)` - 遍历所有配置项

### 2. ConfigVariableBase - 配置变量基类

ConfigVariableBase是所有配置变量的基类，定义了配置项的基本接口。

#### 主要方法

- `virtual std::string toString() = 0` - 将配置项的值转换为字符串
- `virtual bool fromString(const std::string &val) = 0` - 从字符串设置配置项的值
- `virtual std::string getTypeName() const = 0` - 获取配置项值的类型名称
- `const std::string &getName() const` - 获取配置项名称
- `const std::string &getDescription() const` - 获取配置项描述

### 3. ConfigVar - 配置变量模板类

ConfigVar是配置系统的核心类，它封装了特定类型的配置项。通过模板参数可以支持任意数据类型，并提供配置值的自动序列化和反序列化功能。

#### 模板参数

- `T` - 配置项的数据类型
- `fromStr` - 从字符串转换为T类型的转换器，默认使用LexicalCast
- `toStr` - 从T类型转换为字符串的转换器，默认使用LexicalCast

#### 主要方法

- `std::string toString() override` - 将配置项的值转换为字符串表示
- `bool fromString(const std::string &val) override` - 将字符串转换为配置项的值
- `std::string getTypeName() const override` - 获取配置项值的类型名称
- `void setValue(const T &v)` - 设置配置项的值
- `T getValue() const` - 获取配置项的值
- `uint64_t addListener(const ConfigChangeCb &cb)` - 添加配置变更监听器
- `void delListener(uint64_t key)` - 删除指定的配置变更监听器
- `void clearListener()` - 清除所有配置变更监听器
- `ConfigChangeCb getListener(uint64_t key)` - 获取指定的配置变更监听器

### LexicalCast - 词法转换器

LexicalCast是配置值与字符串之间的转换工具，用于实现配置项值与字符串表示之间的相互转换。

#### 特化支持的类型

配置模块原生支持以下类型的配置项：
- 基本数据类型：int、float、double、bool、string等
- 容器类型：vector、list、set、unordered_set、map、unordered_map
- 自定义类型：通过特化LexicalCast实现

## 使用指南

### 基本用法

```cpp
#include "config.hpp"

// 创建或查找一个int类型的配置项
auto port_config = CIM::Config::Lookup<int>("system.port", 8080, "系统端口号");

// 创建或查找一个string类型的配置项
auto name_config = CIM::Config::Lookup<std::string>("system.name", "CIM", "系统名称");

// 获取配置项的值
int port = port_config->getValue();
std::string name = name_config->getValue();

// 设置配置项的值
port_config->setValue(9000);

// 从字符串设置配置项的值
name_config->fromString("new_name");
```

### 配置变更监听

```cpp
#include "config.hpp"

// 创建配置项
auto config_int = CIM::Config::Lookup<int>("test.int", 100, "测试整数配置");

// 添加变更监听器
uint64_t listener_id = config_int->addListener([](const int& old_val, const int& new_val) {
    std::cout << "配置值从 " << old_val << " 变更为 " << new_val << std::endl;
});

// 删除监听器
config_int->delListener(listener_id);
```

### 自定义类型支持

要支持自定义类型，需要特化LexicalCast模板：

```cpp
class Person {
public:
    std::string m_name;
    int m_age;
};

namespace CIM {
    // 特化字符串到Person的转换
    template<>
    class LexicalCast<std::string, Person> {
    public:
        Person operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            return p;
        }
    };

    // 特化Person到字符串的转换
    template<>
    class LexicalCast<Person, std::string> {
    public:
        std::string operator()(const Person& p) {
            YAML::Node node;
            node["name"] = p.m_name;
            node["age"] = p.m_age;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}

// 使用自定义类型配置项
auto person_config = CIM::Config::Lookup<Person>("class.person", Person(), "人员信息");
```

### 配置文件支持

配置模块支持通过YAML配置文件进行配置：

```yaml
system:
  port: 8080
  name: "CIM"
  values: 
    - 1
    - 2
    - 3
  map:
    key1: value1
    key2: value2

class:
  person:
    name: "zhangsan"
    age: 25
```

加载配置文件：

```cpp
#include "config.hpp"
#include <yaml-cpp/yaml.h>

// 加载配置文件
YAML::Node root = YAML::LoadFile("config.yaml");
CIM::Config::LoadFromYaml(root);
```

### 完整示例

```cpp
#include "config.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>

// 定义配置项
auto g_server_port = CIM::Config::Lookup<int>("server.port", 8080, "服务器端口");
auto g_server_name = CIM::Config::Lookup<std::string>("server.name", "MyServer", "服务器名称");

// 配置变更回调函数
void onPortChanged(const int& old_val, const int& new_val) {
    std::cout << "服务器端口从 " << old_val << " 变更为 " << new_val << std::endl;
}

int main() {
    // 添加配置变更监听器
    g_server_port->addListener(onPortChanged);
    
    // 从配置文件加载配置
    try {
        YAML::Node config = YAML::LoadFile("server.yaml");
        CIM::Config::LoadFromYaml(config);
    } catch (...) {
        std::cout << "加载配置文件失败，使用默认配置" << std::endl;
    }
    
    // 使用配置
    std::cout << "服务器名称: " << g_server_name->getValue() << std::endl;
    std::cout << "服务器端口: " << g_server_port->getValue() << std::endl;
    
    // 修改配置
    g_server_port->setValue(9000);
    
    return 0;
}
```

## 设计理念

### 类型安全

配置系统的核心设计理念之一是类型安全。传统的配置系统通常将所有配置项都存储为字符串，使用时再进行类型转换，这种方式容易出错且缺乏编译时检查。

CIM的配置系统通过模板技术实现类型安全：

```cpp
// 类型安全的配置项创建
auto port_config = CIM::Config::Lookup<int>("system.port", 8080, "系统端口号");
auto name_config = CIM::Config::Lookup<std::string>("system.name", "CIM", "系统名称");
```

这样做的好处是：
1. 编译时就能检查类型使用是否正确
2. 避免了运行时类型转换错误
3. 提供了更好的IDE支持和代码提示

### 灵活性与可扩展性

配置系统支持任意数据类型，包括基本类型、STL容器类型，甚至用户自定义类型。这种设计使得系统非常灵活，可以满足各种配置需求。

对于自定义类型，用户只需特化LexicalCast模板即可：

```cpp
namespace CIM {
    template<>
    class LexicalCast<std::string, Person> {
    public:
        Person operator()(const std::string& v) {
            YAML::Node node = YAML::Load(v);
            // 解析逻辑
            return p;
        }
    };
}
```

### 线程安全

作为一个服务框架，线程安全是必不可少的。配置系统通过读写锁（RWMutex）保证了多线程环境下的安全访问：

```cpp
void setValue(const T &v) {
    T old_value;
    std::map<uint64_t, ConfigChangeCb> cbs;
    {
        RWMutexType::WriteLock lock(m_mutex);  // 写锁保护
        // 修改配置值
    }
    // 锁外执行回调，避免死锁
}
```

这种设计既保证了线程安全，又避免了回调函数中可能的死锁问题。

### 核心设计模式

#### 单例模式与静态成员

配置系统使用静态成员变量和函数内静态变量相结合的方式，实现了全局唯一的配置管理器：

```cpp
static ConfigVarMap &GetDatas() {
    static ConfigVarMap m_datas;  // 线程安全的延迟初始化
    return m_datas;
}
```

这种设计解决了静态初始化顺序问题，确保了配置系统在任何时刻都能正常工作。

#### 观察者模式

配置系统实现了观察者模式，支持配置变更时的通知机制：

```cpp
uint64_t addListener(const ConfigChangeCb &cb) {
    static uint64_t func_id = 0;
    RWMutexType::WriteLock lock(m_mutex);
    ++func_id;
    m_cbs[func_id] = cb;
    return func_id;
}
```

这种设计使得系统组件可以动态响应配置变化，实现运行时配置更新。

#### 策略模式

通过模板参数fromStr和toStr，配置系统实现了策略模式，可以灵活地替换字符串转换策略：

```cpp
template <class T,
          class fromStr = LexicalCast<std::string, T>,
          class toStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVariableBase {
    // ...
};
```

### 配置加载与管理

#### YAML集成

配置系统集成了YAML-cpp库，支持从YAML文件加载配置：

```yaml
system:
  port: 8080
  name: "CIM"
  values: 
    - 1
    - 2
    - 3
```

这种设计使得配置文件结构清晰，易于维护和理解。

#### 分层命名空间

配置项支持分层命名，如"system.port"、"database.host"，这种设计提供了良好的组织结构，避免了命名冲突。

### 性能考虑
1. 读写锁优化：使用读写锁而非互斥锁，允许多个读操作并发执行，提高了读多写少场景下的性能。
2. 延迟初始化：采用函数内静态变量实现延迟初始化，只有在真正使用时才创建对象，节省了启动时的资源消耗。
3. 锁外回调：配置变更回调在锁外执行，避免了回调函数中可能的死锁和长时间持有锁的问题。