Sylar 配置模块说明文档
1. 概述
Sylar 配置模块是一个基于 YAML 的配置管理系统，提供了灵活的配置项定义、类型安全的配置访问、配置变更回调通知以及自定义类型支持等功能。该模块采用模板设计，支持多种数据类型，包括基本类型、STL 容器类型和用户自定义类型。

2. 模块结构
配置模块由以下几个核心组件构成：

2.1 核心类结构
sylar
├── ConfigVarBase（配置变量基类）
├── ConfigVar<T>（配置变量模板类）
├── Config（配置管理器）
└── LexicalCast<F, T>（词法转换模板类）
2.2 各组件详细说明
ConfigVarBase（配置变量基类）
所有配置变量的基类，定义了配置项的基本接口：

配置项名称和描述信息
字符串与配置值之间的相互转换接口
获取配置项类型名称的接口
ConfigVar<T>（配置变量模板类）
配置变量的具体实现模板类，支持任意类型 T 的配置项：

支持配置项的创建、读取和修改
提供配置变更回调机制
支持配置值与字符串之间的相互转换
Config（配置管理器）
配置系统的管理类，提供全局配置项管理功能：

配置项的查找和创建
从 YAML 文件加载配置
配置项的全局存储和访问
LexicalCast<F, T>（词法转换模板类）
实现字符串与各种数据类型之间的相互转换：

基本类型转换（通过 boost::lexical_cast 实现）
STL 容器类型转换（vector、list、set、unordered_set、map、unordered_map）
支持用户自定义类型的特化实现
3. 使用说明
3.1 基本使用
cpp
#include "config/config.hpp"

// 定义配置项
auto g_port = sylar::Config::Lookup<int>("system.port", 8080, "系统端口号");
auto g_name = sylar::Config::Lookup<std::string>("system.name", "sylar", "系统名称");

// 读取配置值
int port = g_port->getValue();
std::string name = g_name->getValue();

// 设置配置值
g_port->setValue(9999);

// 转换为字符串
std::string port_str = g_port->toString();
3.2 容器类型配置
配置模块支持多种 STL 容器类型：

cpp
// vector 类型配置
auto g_int_vec = sylar::Config::Lookup<std::vector<int>>("system.int_vec", {1,2,3});

// list 类型配置
auto g_int_list = sylar::Config::Lookup<std::list<int>>("system.int_list", {4,5,6});

// set 类型配置
auto g_int_set = sylar::Config::Lookup<std::set<int>>("system.int_set", {7,8,9});

// map 类型配置
auto g_str_int_map = sylar::Config::Lookup<std::map<std::string, int>>("system.map", {{"key1", 1}, {"key2", 2}});
3.3 配置变更回调
cpp
// 添加配置变更回调
g_port->addListener(1, [](const int& old_value, const int& new_value) {
    std::cout << "配置变更: " << old_value << " -> " << new_value << std::endl;
});

// 删除回调
g_port->delListener(1);
3.4 YAML 配置文件
配置模块支持从 YAML 文件加载配置：

cpp
YAML::Node root = YAML::LoadFile("config.yaml");
sylar::Config::LoadFromYaml(root);
YAML 配置文件示例：

yaml
system:
  port: 8080
  name: "sylar"
  int_vec:
    - 1
    - 2
    - 3
  map:
    key1: 1
    key2: 2
3.5 自定义类型支持
对于自定义类型，需要特化 LexicalCast 模板：

cpp
class Person {
public:
    std::string m_name;
    int m_age;
};

namespace sylar {
    // 字符串到 Person 的转换
    template<>
    class LexicalCast<std::string, Person> {
    public:
        Person operator()(const std::string& v) const {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            return p;
        }
    };

    // Person 到字符串的转换
    template<>
    class LexicalCast<Person, std::string> {
    public:
        std::string operator()(const Person& v) const {
            YAML::Node node;
            node["name"] = v.m_name;
            node["age"] = v.m_age;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}

// 使用自定义类型配置
auto g_person = sylar::Config::Lookup<Person>("system.person", Person());
4. 高级特性
4.1 配置项命名规范
配置项名称只能包含字母、数字、下划线和点号，不区分大小写。

4.2 配置项查找
支持两种查找方式：

Lookup(name, default_value, description) - 查找或创建配置项
Lookup<T>(name) - 仅查找配置项，不存在则返回 nullptr
4.3 类型安全
通过模板和运行时类型检查确保配置项类型安全，防止类型不匹配的错误。

4.4 配置项持久化
支持将配置项序列化为 YAML 格式的字符串，便于存储和传输。

5. 最佳实践
在全局作用域定义配置项，确保在程序启动时就创建
为配置项提供清晰的描述信息，便于维护和文档生成
合理使用配置变更回调，在配置变化时及时更新相关状态
对于复杂配置结构，考虑使用嵌套的 map 或自定义类
使用 YAML 配置文件管理不同环境的配置差异
6. 扩展性
通过特化 LexicalCast 模板支持新的数据类型
可以扩展 ConfigVar 类支持更多功能
可以实现其他格式的配置文件加载器（如 JSON、XML）
以上是 Sylar 配置模块的完整说明文档，通过合理使用该模块，可以为应用程序提供强大而灵活的配置管理功能。