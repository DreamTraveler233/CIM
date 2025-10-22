#pragma once

#include "tcp_server.hpp"
#include "ws_session.hpp"
#include "ws_servlet.hpp"

namespace sylar::http
{
    class WSServer : public TcpServer
    {
    public:
        typedef std::shared_ptr<WSServer> ptr;

        WSServer(sylar::IOManager *worker = sylar::IOManager::GetThis(), sylar::IOManager *io_worker = sylar::IOManager::GetThis(), sylar::IOManager *accept_worker = sylar::IOManager::GetThis());

        WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch; }
        void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v; }

    protected:
        virtual void handleClient(Socket::ptr client) override;

    protected:
        WSServletDispatch::ptr m_dispatch;
    };
}