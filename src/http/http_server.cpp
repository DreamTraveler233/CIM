#include "http_server.hpp"
#include "macro.hpp"
// #include "config_servlet.hpp"
// #include "status_servlet.hpp"

namespace CIM::http
{
    static CIM::Logger::ptr g_logger = CIM_LOG_NAME("system");

    HttpServer::HttpServer(bool keepalive, IOManager *worker, IOManager *accept_worker)
        : TcpServer(worker, accept_worker),
          m_isKeepalive(keepalive)
    {
        m_dispatch.reset(new ServletDispatch);

        // m_type = "http";
        // m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
        // m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
    }

    ServletDispatch::ptr HttpServer::getServletDispatch() const { return m_dispatch; }

    void HttpServer::setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

    // void HttpServer::setName(const std::string &v)
    // {
    //     TcpServer::setName(v);
    //     m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
    // }

    /**
     * @brief 处理客户端连接请求
     * @param client 客户端Socket连接对象
     *
     * 该函数负责处理单个客户端的HTTP请求，包括接收请求、处理请求、发送响应等完整流程。
     * 支持HTTP长连接，可以处理同一个客户端的多个连续请求。
     */
    void HttpServer::handleClient(Socket::ptr client)
    {
        CIM_LOG_DEBUG(g_logger) << "handleClient " << *client;
        // 创建HTTP会话对象，用于管理与客户端的通信
        HttpSession::ptr session(new HttpSession(client));
        do
        {
            // 接收客户端的HTTP请求
            auto req = session->recvRequest();
            if (!req)
            {
                // 请求接收失败，记录错误日志并退出循环
                CIM_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                                          << errno << " errstr=" << strerror(errno)
                                          << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
                break;
            }

            // 创建HTTP响应对象，根据请求版本和连接状态设置是否保持连接
            HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_isKeepalive));
            // 设置服务器标识头
            rsp->setHeader("Server", getName());
            // 调度器处理请求并生成响应
            m_dispatch->handle(req, rsp, session);
            // 发送HTTP响应给客户端
            session->sendResponse(rsp);

            // 检查是否需要关闭连接：如果服务器不支持长连接或客户端请求关闭连接，则退出循环
            if (!m_isKeepalive || req->isClose())
            {
                break;
            }
        } while (true);
        // 关闭会话连接
        session->close();
    }
}