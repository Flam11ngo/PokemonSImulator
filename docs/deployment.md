# PokemonSimulator 部署指南

## 架构

```
                 nginx :80 (可选)
                /          \
    Vite :5173 (开发)       Python :9000 (API + WebSocket)
         |                        |
    frontend/               standalone_server.py
                                 |
                          Node.js Showdown daemon
                          (engine-adapter/showdown_daemon.js)
```

## 依赖

| 组件 | 版本要求 | 用途 |
|------|---------|------|
| Node.js | ≥ 20 LTS | 前端 Vite + Showdown 对战引擎 |
| Python | ≥ 3.11 | API + WebSocket 服务器 |
| nginx | 任意版本 | 反向代理 (可选，开发模式可跳过) |
| npm | 随 Node.js | 包管理 |

## 快速启动 (开发模式)

### Windows

```powershell
# 1. 安装前端依赖
cd frontend
npm install

# 2. 安装 Python 依赖
cd ..\api-server
pip install fastapi uvicorn

# 3. 启动后端 (端口 9000)
start "API" cmd /c "cd api-server && python standalone_server.py"

# 4. 启动前端 (端口 5173)
start "Vite" cmd /c "cd frontend && npx vite --host 0.0.0.0"
```

访问 `http://localhost:5173`

### Linux / macOS

```bash
# 1. 依赖
cd frontend && npm install
pip install fastapi uvicorn

# 2. 启动
cd api-server && python standalone_server.py &
cd frontend && npx vite --host 0.0.0.0 &
```

## 生产部署 (nginx)

### 1. 构建前端

```bash
cd frontend
npm run build
# 输出: frontend/dist/
```

### 2. Nginx 配置

将 `nginx/nginx.conf` 复制到 nginx 的 `conf/` 目录，修改 `root` 指向 `frontend/dist`。

```nginx
server {
    listen 80;
    root /path/to/frontend/dist;
    index index.html;

    location /ws {
        proxy_pass http://127.0.0.1:9000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
    }

    location /api/ {
        proxy_pass http://127.0.0.1:9000;
    }

    location / {
        try_files $uri $uri/ /index.html;
    }
}
```

### 3. 启动服务

```bash
# 后端
cd api-server && python standalone_server.py &

# nginx
nginx
```

访问 `http://localhost`

## 进程管理 (Linux, 使用 systemd)

```ini
# /etc/systemd/system/pokemon-api.service
[Unit]
Description=PokemonSimulator API Server
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/PokemonSImulator/api-server
ExecStart=/usr/bin/python standalone_server.py
Restart=always

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl enable --now pokemon-api
```

## 数据文件

| 文件 | 用途 | 生成方式 |
|------|------|---------|
| `data/pokemon.db` | 游戏数据 | `python scripts/py/rebuild_db.py` |
| `smogon_stats/gen91v1_stats.sqlite` | 环境统计 | `python smogon_stats/convert.py` |
| `frontend/public/icons-sheet.png` | 精灵图标 | `python scripts/py/rebuild_sheets.py` |

## Kafka (可选)

如需将分析数据发送到 Hadoop/Kafka:

```bash
# 创建 topic
kafka-topics.sh --create --topic player.ui.events --bootstrap-server 192.168.88.129:9092
kafka-topics.sh --create --topic battle.logs --bootstrap-server 192.168.88.129:9092

# 验证
kafka-console-consumer.sh --topic battle.logs --from-beginning --bootstrap-server 192.168.88.129:9092
```

Kafka 不可用时，分析数据仅写本地 `logs/analytics/`，不影响对战。
