#ifndef __CIM_HTTP_SERVLETS_CONFIG_SERVLET_HPP__
#define __CIM_HTTP_SERVLETS_CONFIG_SERVLET_HPP__

#include "http_servlet.hpp"

namespace CIM::http
{

    class ConfigServlet : public Servlet
    {
    public:
        ConfigServlet();
        virtual int32_t handle(HttpRequest::ptr request,
                               HttpResponse::ptr response,
                               HttpSession::ptr session) override;
    };

}

#endif