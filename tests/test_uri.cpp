#include "uri.hpp"
#include <iostream>

void test(const std::string &str)
{
    sylar::Uri::ptr uri = sylar::Uri::Create(str);
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
}

int main(int argc, char **argv)
{
    test("http://www.sylar.top/test/uri?id=100&name=sylar#frg");
    test("http://admin@www.sylar.top/test/中文/uri?id=100&name=sylar&vv=中文#frg中文");
    test("http://admin@www.sylar.top");
    test("https://www.baidu.com");
    return 0;
}