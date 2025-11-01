#include "api/ws_gateway_module.hpp"

#include <jwt-cpp/jwt.h>

#include <atomic>
#include <unordered_map>

#include "base/macro.hpp"
#include "common/common.hpp"
#include "http/ws_server.hpp"
#include "http/ws_servlet.hpp"
#include "http/ws_session.hpp"
#include "system/application.hpp"
#include "util/util.hpp"

namespace CIM::api {

static auto g_logger = CIM_LOG_NAME("root");

WsGatewayModule::WsGatewayModule() : Module("ws.gateway", "0.1.0", "builtin") {}

// 简易查询串解析（假设无需URL解码，前端传递 token 直接可用）
static std::unordered_map<std::string, std::string> ParseQueryKV(const std::string& q) {
    std::unordered_map<std::string, std::string> kv;
    if (q.empty()) return kv;
    size_t start = 0;
    while (start < q.size()) {
        size_t amp = q.find('&', start);
        if (amp == std::string::npos) amp = q.size();
        size_t eq = q.find('=', start);
        if (eq != std::string::npos && eq < amp) {
            std::string k = q.substr(start, eq - start);
            std::string v = q.substr(eq + 1, amp - (eq + 1));
            kv[k] = v;
        } else {
            // 只有key没有value
            std::string k = q.substr(start, amp - start);
            if (!k.empty()) kv[k] = "";
        }
        start = amp + 1;
    }
    return kv;
}

// 连接上下文与会话表（首版进程内，支持多连接）
struct ConnCtx {
    uint64_t uid = 0;
    std::string platform;  // web|pc|app，默认 web
    std::string conn_id;   // 连接唯一ID
};

static std::atomic<uint64_t> s_conn_seq{1};
static CIM::RWMutex s_ws_mutex;                        // 保护会话表
static std::unordered_map<void*, ConnCtx> s_ws_conns;  // key: WSSession* 原始地址

// 发送下行统一封装：{"event":"...","payload":{...},"ackid":"..."}
static void SendEvent(CIM::http::WSSession::ptr session, const std::string& event,
                      const Json::Value& payload, const std::string& ackid = "") {
    Json::Value root;
    root["event"] = event;
    root["payload"] = payload.isNull() ? Json::Value(Json::objectValue) : payload;
    if (!ackid.empty()) root["ackid"] = ackid;
    session->sendMessage(CIM::JsonUtil::ToString(root));
}

bool WsGatewayModule::onServerReady() {
    std::vector<CIM::TcpServer::ptr> wsServers;
    // 1. 获取所有已注册的WebSocket服务器实例
    if (!CIM::Application::GetInstance()->getServer("ws", wsServers)) {
        CIM_LOG_WARN(g_logger) << "no ws servers found when registering ws routes";
        return true;
    }

    // 2. 遍历每个WebSocket服务器，注册路由与事件回调
    for (auto& s : wsServers) {
        auto ws = std::dynamic_pointer_cast<CIM::http::WSServer>(s);
        if (!ws) continue;
        auto dispatch = ws->getWSServletDispatch();

        /* 注册 WebSocket 路由回调 */
        // 2.1 连接建立回调：鉴权、会话登记、欢迎包
        auto on_connect = [](CIM::http::HttpRequest::ptr header,
                             CIM::http::WSSession::ptr session) -> int32_t {
            // 读取查询串 ?token=...&platform=...
            const std::string query = header->getQuery();
            auto kv = ParseQueryKV(query);
            const std::string token = CIM::GetParamValue<std::string>(kv, "token", "");
            std::string platform = CIM::GetParamValue<std::string>(kv, "platform", "web");
            std::string suid;

            // 1) 校验token（JWT），失败则发错误事件并关闭
            if (token.empty() || !VerifyJwt(token, &suid) || suid.empty()) {
                Json::Value err;
                err["error_code"] = 401;
                err["error_message"] = "unauthorized";
                SendEvent(session, "event_error", err);
                return -1;  // 触发上层关闭
            }

            // 2) 解析uid，校验合法性
            uint64_t uid = 0;
            try {
                uid = std::stoull(suid);
            } catch (...) {
                uid = 0;
            }
            if (uid == 0) {
                Json::Value err;
                err["error_code"] = 401;
                err["error_message"] = "invalid uid";
                SendEvent(session, "event_error", err);
                return -1;
            }

            // 3) 构造连接上下文，写锁保护下登记到全局会话表
            ConnCtx ctx;
            ctx.uid = uid;
            ctx.platform = platform.empty() ? std::string("web") : platform;
            ctx.conn_id = std::to_string(s_conn_seq.fetch_add(1));

            {
                CIM::RWMutex::WriteLock lock(s_ws_mutex);
                s_ws_conns[(void*)session.get()] = ctx;
            }

            // 4) 发送欢迎包，event="connect"
            Json::Value payload;
            payload["uid"] = Json::UInt64(uid);
            payload["platform"] = ctx.platform;
            payload["ts"] = (Json::UInt64)CIM::TimeUtil::NowToMS();
            SendEvent(session, "connect", payload);
            return 0;
        };

        // 2.2 连接关闭回调：移除会话表
        auto on_close = [](CIM::http::HttpRequest::ptr /*header*/,
                           CIM::http::WSSession::ptr session) -> int32_t {
            CIM::RWMutex::WriteLock lock(s_ws_mutex);
            s_ws_conns.erase((void*)session.get());
            return 0;
        };

        // 2.3 消息处理回调：事件分发、心跳、回显等
        auto on_message = [](CIM::http::HttpRequest::ptr /*header*/,
                             CIM::http::WSFrameMessage::ptr msg,
                             CIM::http::WSSession::ptr session) -> int32_t {
            // 仅处理文本帧，忽略二进制和控制帧
            if (!msg || msg->getOpcode() != CIM::http::WSFrameHead::TEXT_FRAME) {
                return 0;
            }
            const std::string& data = msg->getData();
            Json::Value root;
            // 1) 解析JSON消息体，非对象型忽略
            if (!CIM::JsonUtil::FromString(root, data) || !root.isObject()) {
                return 0;  // 非JSON忽略
            }
            // 前端封装为 {"event": event, "payload": payload}
            const std::string event = CIM::JsonUtil::GetString(root, "event");
            const Json::Value payload =
                root.isMember("payload") ? root["payload"] : Json::Value(Json::objectValue);

            // 2) 内置事件处理
            if (event == "ping") {
                // 应用层心跳，回复pong
                Json::Value p;
                p["ts"] = (Json::UInt64)CIM::TimeUtil::NowToMS();
                SendEvent(session, "pong", p);
                return 0;
            }
            if (event == "ack") {
                // 收到ACK，可做去重确认，这里暂忽略
                return 0;
            }
            if (event == "echo") {
                // 回显测试
                SendEvent(session, "echo", payload);
                return 0;
            }

            // 3) 其他事件留给后续业务模块拓展，这里仅记录日志
            CIM_LOG_DEBUG(g_logger) << "unhandled ws event: " << event;
            return 0;
        };

        // 2.4 注册路径：固定 /wss/default.io，同时开放 /wss/* 以便未来扩展
        dispatch->addServlet("/wss/default.io", on_message, on_connect, on_close);
        dispatch->addGlobServlet("/wss/*", on_message, on_connect, on_close);
    }

    // 3. 日志记录路由注册完成
    CIM_LOG_INFO(g_logger) << "ws routes registered";
    return true;
}

}  // namespace CIM::api
