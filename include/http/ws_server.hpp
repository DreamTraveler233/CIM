#pragma once

#include "tcp_server.hpp"
#include "ws_session.hpp"
#include "ws_servlet.hpp"

namespace CIM::http
{
    class WSServer : public TcpServer
    {
    public:
        typedef std::shared_ptr<WSServer> ptr;

        WSServer(IOManager *worker = IOManager::GetThis(),
                 IOManager *io_worker = IOManager::GetThis(),
                 IOManager *accept_worker = IOManager::GetThis());

        WSServletDispatch::ptr getWSServletDispatch() const { return m_dispatch; }
        void setWSServletDispatch(WSServletDispatch::ptr v) { m_dispatch = v; }

    protected:
        virtual void handleClient(Socket::ptr client) override;

    protected:
        WSServletDispatch::ptr m_dispatch;
    };
}