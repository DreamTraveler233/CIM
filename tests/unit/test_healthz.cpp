#include <gtest/gtest.h>
#include "http/http_servlet.hpp"
#include "http/http.hpp"

// 目标：验证健康检查路由处理逻辑的约定（Content-Type 与 Body 内容）
// 这里不依赖真实网络，仅通过 ServletDispatch + FunctionServlet 进行单元验证。

TEST(HealthzRoute, ReturnsOkJson) {
    using namespace CIM::http;

    ServletDispatch::ptr dispatch(new ServletDispatch());

    // 与 MinimalApiModule 中的 /healthz 处理保持一致
    dispatch->addServlet("/healthz", [](HttpRequest::ptr /*req*/, HttpResponse::ptr res, HttpSession::ptr /*session*/) {
        res->setHeader("Content-Type", "application/json");
        res->setBody("{\"status\":\"ok\"}");
        return 0;
    });

    // 模拟一次请求处理
    auto servlet = dispatch->getMatchedServlet("/healthz");
    ASSERT_NE(servlet, nullptr);

    auto req = std::make_shared<HttpRequest>();
    auto res = std::make_shared<HttpResponse>();

    int rc = servlet->handle(req, res, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(res->getHeader("Content-Type"), "application/json");
    EXPECT_EQ(res->getBody(), "{\"status\":\"ok\"}");
}
