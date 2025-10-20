#include "http_server.hpp"
#include "macro.hpp"
#include "servlet.hpp"

void test()
{
    sylar::http::HttpServer::ptr http_server(new sylar::http::HttpServer);
    sylar::Address::ptr addr = sylar::Address::LookupAnyIpAddress("0.0.0.0:8020");
    while (!http_server->bind(addr))
    {
        sleep(2);
    }

    auto sb = http_server->getServletDispatch();
    sb->addServlet("/sylar/xx", [](sylar::http::HttpRequest::ptr req,
                                   sylar::http::HttpResponse::ptr res,
                                   sylar::http::HttpSession::ptr session)
                   { 
                    res->setBody(req->toString()); 
                    return 0; });

    sb->addGlobServlet("/sylar/*", [](sylar::http::HttpRequest::ptr req,
                                      sylar::http::HttpResponse::ptr res,
                                      sylar::http::HttpSession::ptr session)
                       { 
                        res->setBody("Glob: \r\n" + req->toString());
                        return 0; });

    http_server->start();
}

int main(int argc, char **argv)
{
    sylar::IOManager iom(2);
    iom.schedule(test);
    return 0;
}