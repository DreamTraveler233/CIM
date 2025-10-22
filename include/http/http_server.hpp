#pragma once

#include "tcp_server.hpp"
#include "http_session.hpp"
#include "http_servlet.hpp"

namespace sylar::http
{
    /**
     * @brief HTTP服务器类
     */
    class HttpServer : public TcpServer
    {
    public:
        /// 智能指针类型
        typedef std::shared_ptr<HttpServer> ptr;

        /**
         * @brief 构造函数
         * @param[in] keepalive 是否长连接
         * @param[in] worker 工作调度器
         * @param[in] accept_worker 接收连接调度器
         */
        HttpServer(bool keepalive = false,
                   IOManager *worker = IOManager::GetThis(),
                   IOManager *accept_worker = IOManager::GetThis());

        /**
         * @brief 获取ServletDispatch
         */
        ServletDispatch::ptr getServletDispatch() const;

        /**
         * @brief 设置ServletDispatch
         */
        void setServletDispatch(ServletDispatch::ptr v);

        // virtual void setName(const std::string &v) override;

    protected:
        virtual void handleClient(Socket::ptr client) override;

    private:
        bool m_isKeepalive;              /// 是否支持长连接
        ServletDispatch::ptr m_dispatch; /// Servlet分发器
    };
}