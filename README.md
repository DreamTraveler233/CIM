# CIM 开发环境指南（含 CORS 开发代理）

本项目在开发期推荐通过本地 Nginx 作为“开发代理”统一处理 CORS 和预检请求，并将 `/api`、`/wss` 转发到后端。

- 配置文件：`scripts/docker/nginx.dev.conf`
- 控制脚本：`scripts/docker/nginx-dev.sh`

## 一、启动后端

确保你能启动后端（HTTP: 8080，WebSocket: 8081）：

- 二进制：`bin/cim_server -s`
- 配置（示例）：`bin/config/server.yaml`

## 二、启动 Nginx 开发代理

推荐使用脚本（无需 root，不影响系统 nginx）：

```bash
# 启动
./scripts/docker/nginx-dev.sh start

# 重载配置
./scripts/docker/nginx-dev.sh reload

# 停止
./scripts/docker/nginx-dev.sh stop

# 查看状态
./scripts/docker/nginx-dev.sh status

# 校验配置
./scripts/docker/nginx-dev.sh test

# 实时查看日志
./scripts/docker/nginx-dev.sh logs
```

重要说明：脚本会使用同目录下的 `nginx.dev.conf` 和 `nginx.pid`，通过发送信号控制“开发实例”，不会去操作系统 nginx。错误日志写入 `scripts/docker/nginx.error.log`，访问日志写入 `scripts/docker/nginx.access.log`。

如需直接用 nginx 命令：

```bash
# 启动指定配置（与脚本等价）
nginx -c /home/szy/code/CIM/scripts/docker/nginx.dev.conf

# 重载（指定 PID 文件，避免误打到系统实例）
nginx -s reload -g 'pid /home/szy/code/CIM/scripts/docker/nginx.pid;'

# 或直接给指定 PID 发 HUP/QUIT 信号
kill -HUP  $(cat /home/szy/code/CIM/scripts/docker/nginx.pid)
kill -QUIT $(cat /home/szy/code/CIM/scripts/docker/nginx.pid)
```

## 三、前端对接

将前端 .env 设置为指向开发代理（注意路径前缀与 WS 路径）：

```dotenv
# 示例（Vite）
ENV=development
VITE_BASE=/
VITE_ROUTER_MODE=history
VITE_BASE_API=http://localhost:9000/api
VITE_SOCKET_API=ws://localhost:9000/wss

# PEM 建议使用 \n 进行转义
VITE_RSA_PUBLIC_KEY="-----BEGIN RSA PUBLIC KEY-----\n...\n-----END RSA PUBLIC KEY-----\n"
```

若前端实际端口不是 5173/3000，请在 `nginx.dev.conf` 的 `map $http_origin $cors_allow_origin` 白名单中添加你的来源。

## 四、快速自检

- 主页占位：

```bash
curl -i http://localhost:9000/
```

- 预检请求（应返回 204，并带 Access-Control-Allow-*）：

```bash
curl -i -X OPTIONS \
  -H "Origin: http://localhost:5173" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: content-type,authorization" \
  http://localhost:9000/api/v1/auth/login
```

- 实际请求（应 200，并带回显的 Allow-Origin）：

```bash
curl -i -X POST \
  -H "Origin: http://localhost:5173" \
  -H "Content-Type: application/json" \
  -d '{"mobile":"18800000000","password":"xxx"}' \
  http://localhost:9000/api/v1/auth/login
```

## 五、常见问题

- 提示 `could not open error log file: /var/log/nginx/error.log (Permission denied)`：
  - 我们的配置已将日志写入仓库目录（`scripts/docker/nginx.error.log`）。若仍看到该报错，说明你在对系统 nginx 执行 `nginx -s reload`。请使用脚本或在命令中指定本实例的 pid：
    - `nginx -s reload -g 'pid /home/szy/code/CIM/scripts/docker/nginx.pid;'`

- 提示找不到 `mime.types`：
  - 配置已改为 `include /etc/nginx/mime.types;`。若你的系统不存在该文件，请手动安装 nginx-common 或把最小 `types` 块内联进配置。

- CORS 与凭证：
  - 需要 Cookie 或 `Authorization` 时，Nginx 会回显具体 Origin 并开启 `Access-Control-Allow-Credentials: true`，请在前端开启凭证传递（如 fetch `credentials: 'include'`，axios `withCredentials = true`）。

## 六、结构参考

- 代理配置：`scripts/docker/nginx.dev.conf`
- 管理脚本：`scripts/docker/nginx-dev.sh`
- 后端配置：`bin/config/server.yaml`（HTTP:8080 / WS:8081）

如需将你的前端来源加入白名单、调整 WebSocket 路径或添加 Docker 方式的 Nginx 运行示例，请提交 issue 或直接修改上述文件。