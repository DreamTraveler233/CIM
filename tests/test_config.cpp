#include "config.hpp"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <cassert>

/**
 * 查询在已存储的配置项中，是否存在 int 类型且配置项名称为 system.port 的配置项，
 * 如果不存在则创建该配置项，并设默认值为 8080，配置项描述为 system port number
 */
// 定义配置项
auto config_int = sylar::Config::Lookup<int>("system.port", 8080, "system port number");
auto config_float = sylar::Config::Lookup<float>("system.value", 10.5f, "system value");
auto config_string = sylar::Config::Lookup<std::string>("system.name", "sylar", "system name");
auto config_int_vector = sylar::Config::Lookup<std::vector<int>>("system.int_ver", std::vector<int>{1, 2}, "system int vec");
auto config_int_list = sylar::Config::Lookup<std::list<int>>("system.int_list", std::list<int>{4, 5, 6}, "system int list");
auto config_int_set = sylar::Config::Lookup<std::set<int>>("system.int_set", std::set<int>{99, 100, 101}, "system int set");
auto config_int_unordered_set = sylar::Config::Lookup<std::unordered_set<int>>("system.int_unordered_set", std::unordered_set<int>{233, 244, 255}, "system int unordered_set");
auto config_int_map = sylar::Config::Lookup<std::map<std::string, int>>("system.int_map", std::map<std::string, int>{{"k", 2}}, "system int map");
auto config_int_unordered_map = sylar::Config::Lookup<std::unordered_map<std::string, int>>("system.int_unordered_map", std::unordered_map<std::string, int>{{"k1", 1}, {"k2", 2}, {"k3", 3}}, "system int unordered_map");

// 测试配置项的基本功能
void test_config_basic()
{
    std::cout << "=================== 测试配置项基本功能 ===================" << std::endl;

    // 测试配置项的初始值
    assert(config_int->getValue() == 8080);
    assert(config_float->getValue() == 10.5f);
    assert(config_string->getValue() == "sylar");

    std::cout << "配置项初始值测试通过" << std::endl;

    // 测试配置项的toString功能
    assert(config_int->toString() == "8080");
    assert(config_float->toString() == "10.5");
    assert(config_string->toString() == "sylar");

    std::cout << "配置项toString功能测试通过" << std::endl;

    // 测试配置项的fromString功能
    config_int->fromString("9999");
    assert(config_int->getValue() == 9999);

    config_string->fromString("new_name");
    assert(config_string->getValue() == "new_name");

    std::cout << "配置项fromString功能测试通过" << std::endl;

    // 测试重复名称但不同类型的情况
    try
    {
        auto config_int_error = sylar::Config::Lookup<float>("system.port", 8080.0f, "error config");
        assert(false); // 不应该执行到这里
    }
    catch (...)
    {
        std::cout << "重复名称不同类型配置项测试通过" << std::endl;
    }
}

// 测试配置项的容器类型
void test_config_containers()
{
    std::cout << "=================== 测试配置项容器类型 ===================" << std::endl;

    // 测试vector
    auto vec = config_int_vector->getValue();
    assert(vec.size() == 2);
    assert(vec[0] == 1 && vec[1] == 2);
    std::cout << "vector配置项测试通过" << std::endl;

    // 测试list
    auto list = config_int_list->getValue();
    assert(list.size() == 3);
    auto list_it = list.begin();
    assert(*list_it == 4);
    list_it++;
    assert(*list_it == 5);
    list_it++;
    assert(*list_it == 6);
    std::cout << "list配置项测试通过" << std::endl;

    // 测试set
    auto set = config_int_set->getValue();
    assert(set.size() == 3);
    assert(set.find(99) != set.end());
    assert(set.find(100) != set.end());
    assert(set.find(101) != set.end());
    std::cout << "set配置项测试通过" << std::endl;

    // 测试unordered_set
    auto uset = config_int_unordered_set->getValue();
    assert(uset.size() == 3);
    assert(uset.find(233) != uset.end());
    assert(uset.find(244) != uset.end());
    assert(uset.find(255) != uset.end());
    std::cout << "unordered_set配置项测试通过" << std::endl;

    // 测试map
    auto map = config_int_map->getValue();
    assert(map.size() == 1);
    assert(map.at("k") == 2);
    std::cout << "map配置项测试通过" << std::endl;

    // 测试unordered_map
    auto umap = config_int_unordered_map->getValue();
    assert(umap.size() == 3);
    assert(umap.at("k1") == 1);
    assert(umap.at("k2") == 2);
    assert(umap.at("k3") == 3);
    std::cout << "unordered_map配置项测试通过" << std::endl;
}

// 测试配置变更回调功能
void test_config_callback()
{
    std::cout << "=================== 测试配置变更回调功能 ===================" << std::endl;

    int callback_count = 0;
    int old_value = 0;
    int new_value = 0;

    // 添加回调函数
    config_int->addListener([&callback_count, &old_value, &new_value](const int &old_val, const int &new_val)
                            {
                                callback_count++;
                                old_value = old_val;
                                new_value = new_val; });

    // 修改配置值
    int original_value = config_int->getValue();
    config_int->setValue(12345);

    // 检查回调是否被调用
    assert(callback_count == 1);
    assert(old_value == original_value);
    assert(new_value == 12345);
    assert(config_int->getValue() == 12345);

    std::cout << "配置变更回调功能测试通过" << std::endl;

    // 测试删除回调
    config_int->delListener(1);
    config_int->setValue(54321);
    // 回调计数应该保持不变
    assert(callback_count == 1);

    std::cout << "配置回调删除功能测试通过" << std::endl;
}

// 测试YAML配置加载
void test_yaml_load()
{
    std::cout << "=================== 测试YAML配置加载 ===================" << std::endl;

    // 保存原始值
    // int original_port = config_int->getValue();

    // 加载配置
    YAML::Node root = YAML::LoadFile("/home/szy/code/sylar/bin/config/test.yaml");
    sylar::Config::LoadFromYaml(root);

    // 检查值是否正确更新
    assert(config_int->getValue() == 9999);
    assert(config_float->getValue() == 15.0f);

    // 检查vector是否正确更新
    auto vec = config_int_vector->getValue();
    assert(vec.size() == 4);
    assert(vec[0] == 1 && vec[1] == 2 && vec[2] == 3 && vec[3] == 4);

    // 检查list是否正确更新
    auto list = config_int_list->getValue();
    assert(list.size() == 2);
    auto list_it = list.begin();
    assert(*list_it == 90);
    list_it++;
    assert(*list_it == 80);

    std::cout << "YAML配置加载测试通过" << std::endl;
}

class Person
{
public:
    std::string toString() const
    {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person &p) const
    {
        return p.m_name == m_name && p.m_age == m_age && p.m_sex == m_sex;
    }

    std::string m_name = "";
    int m_age = 0;
    bool m_sex = 0;
};

namespace sylar
{
    template <>
    class LexicalCast<std::string, Person>
    {
    public:
        Person operator()(const std::string &v) const
        {
            YAML::Node node = YAML::Load(v);
            Person p;
            p.m_name = node["name"].as<std::string>();
            p.m_age = node["age"].as<int>();
            p.m_sex = node["sex"].as<bool>();
            return p;
        }
    };

    template <>
    class LexicalCast<Person, std::string>
    {
    public:
        std::string operator()(const Person &v) const
        {
            YAML::Node node;
            node["name"] = v.m_name;
            node["age"] = v.m_age;
            node["sex"] = v.m_sex;
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
}

auto person = sylar::Config::Lookup("class.person", Person(), "person");

// 测试自定义类型配置
void test_custom_type()
{
    std::cout << "=================== 测试自定义类型配置 ===================" << std::endl;

    // 当未加载过配置文件时才等于空值
    // assert(person->getValue() == Person()); // 初始值为空

    // 加载配置
    YAML::Node root = YAML::LoadFile("/home/szy/code/sylar/bin/config/test.yaml");
    sylar::Config::LoadFromYaml(root);

    auto loaded_person = person->getValue();
    assert(loaded_person.m_name == "zhangsan");
    assert(loaded_person.m_age == 22);
    assert(loaded_person.m_sex == true);

    std::cout << "自定义类型配置测试通过" << std::endl;
}

int main(int argc, char **argv)
{
    std::cout << "开始执行配置模块全面测试" << std::endl;

    test_config_basic();
    test_config_containers();
    test_config_callback();
    test_yaml_load();
    test_custom_type();

    std::cout << "=================== 配置模块所有测试通过 ===================" << std::endl;
    return 0;
}