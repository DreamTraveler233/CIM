#include "api/minimal_api_module.hpp"

#include "base/macro.hpp"
#include "http/http_server.hpp"
#include "http/http_servlet.hpp"
#include "system/application.hpp"

namespace CIM::api
{
    static auto g_logger = CIM_LOG_NAME("api");

    MinimalApiModule::MinimalApiModule()
        : Module("api.minimal", "0.1.0", "builtin")
    {
    }

    bool MinimalApiModule::onServerReady()
    {
        /* 获取所有 HTTP 服务器实例 */
        std::vector<CIM::TcpServer::ptr> httpServers;
        if (!CIM::Application::GetInstance()->getServer("http", httpServers))
        {
            CIM_LOG_WARN(g_logger) << "no http servers found when registering minimal routes";
            return true;
        }

        /* 注册 HTTP 路由 */
        for (auto &s : httpServers)
        {
            auto http = std::dynamic_pointer_cast<CIM::http::HttpServer>(s);
            if (!http)
                continue;
            auto dispatch = http->getServletDispatch();

            dispatch->addServlet("/healthz", [](CIM::http::HttpRequest::ptr /*req*/,
                                                CIM::http::HttpResponse::ptr res,
                                                CIM::http::HttpSession::ptr /*session*/)
                                 {
      res->setHeader("Content-Type", "application/json");
      res->setBody("{\"status\":\"ok\"}");
      return 0; });

            dispatch->addServlet("/readyz", [](CIM::http::HttpRequest::ptr /*req*/,
                                               CIM::http::HttpResponse::ptr res,
                                               CIM::http::HttpSession::ptr /*session*/)
                                 {
      res->setHeader("Content-Type", "application/json");
      res->setBody("{\"ready\":true}");
      return 0; });

            dispatch->addServlet("/api/v1/ping", [](CIM::http::HttpRequest::ptr /*req*/,
                                                    CIM::http::HttpResponse::ptr res,
                                                    CIM::http::HttpSession::ptr /*session*/)
                                 {
      res->setHeader("Content-Type", "application/json");
      res->setBody("{\"code\":0,\"msg\":\"ok\"}");
      return 0; });
        }

        CIM_LOG_INFO(g_logger) << "minimal routes registered: /healthz, /readyz, /api/v1/ping";
        return true;
    }

} // namespace CIM::api
