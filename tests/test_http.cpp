#include "http.hpp"
#include "macro.hpp"
#include <iostream>

static auto g_logger = SYLAR_LOG_ROOT();

void test_request()
{
    sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
    req->setHeader("host", "www.baidu.com");
    std::cout << req->toString() << std::endl;
}

void test_response()
{
    sylar::http::HttpResponse::ptr req(new sylar::http::HttpResponse);
    req->setHeader("X-X", "sylar");
    req->setBody("hello world");
    req->setStatus((sylar::http::HttpStatus)404);
    req->setClose(false);
    std::cout << req->toString() << std::endl;
}

int main(int argc, char **crgv)
{
    test_request();
    test_response();
    return 0;
}