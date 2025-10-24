#include "uri.hpp"
#include <iostream>

void test(const std::string &str)
{
    CIM::Uri::ptr uri = CIM::Uri::Create(str);
    std::cout << uri->toString() << std::endl;
    auto addr = uri->createAddress();
    std::cout << *addr << std::endl;
}

int main(int argc, char **argv)
{
    test("http://www.CIM.top/test/uri?id=100&name=CIM#frg");
    test("http://admin@www.CIM.top/test/中文/uri?id=100&name=CIM&vv=中文#frg中文");
    test("http://admin@www.CIM.top");
    test("https://www.baidu.com");
    return 0;
}