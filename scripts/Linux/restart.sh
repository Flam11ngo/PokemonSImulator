#!/bin/bash
# PokemonSimulator 服务重启脚本 — 允许局域网访问
set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
LOG_DIR="$PROJECT_DIR/logs"
FRONTEND_DIR="$PROJECT_DIR/frontend"
mkdir -p "$LOG_DIR"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok() { echo -e "${GREEN}  ✅ $1${NC}"; }
warn() { echo -e "${YELLOW}  ⚠️  $1${NC}"; }
fail() { echo -e "${RED}  ❌ $1${NC}"; exit 1; }

# Get LAN IP (WSL: use Windows host IP via /etc/resolv.conf)
if grep -qi microsoft /proc/version 2>/dev/null; then
    LAN_IP=$(grep nameserver /etc/resolv.conf | awk '{print $2}' | head -1)
else
    LAN_IP=$(hostname -I 2>/dev/null | awk '{print $1}')
fi
[ -z "$LAN_IP" ] && LAN_IP=$(ip route get 1 2>/dev/null | awk '{print $7; exit}')
[ -z "$LAN_IP" ] && LAN_IP="0.0.0.0"

echo "========================================"
echo "  PokemonSimulator 服务重启"
echo "========================================"

# ---- 1. 停止旧服务 ----
echo "[1/4] 停止旧服务..."
fuser -k 8000/tcp 2>/dev/null && ok "API Server (8000) 已停止" || echo "  API Server 未运行"
fuser -k 5173/tcp 2>/dev/null && ok "Frontend (5173) 已停止" || echo "  Frontend 未运行"
sleep 1

# ---- 2. 启动 API Server ----
echo "[2/4] 启动 API Server..."
cd "$PROJECT_DIR"
nohup python3 api-server/standalone_server.py > "$LOG_DIR/api-server.log" 2>&1 &
API_PID=$!

for i in {1..15}; do
    sleep 0.5
    if curl -sf http://localhost:8000/api/v1/health > /dev/null 2>&1; then
        ok "API Server 就绪 (PID=$API_PID)"
        break
    fi
    if [ $i -eq 15 ]; then
        fail "API Server 启动超时，查看日志: $LOG_DIR/api-server.log"
    fi
done

# ---- 3. 启动前端 ----
echo "[3/4] 启动 Vue 前端..."
cd "$FRONTEND_DIR"
nohup npm run dev -- --host 0.0.0.0 > "$LOG_DIR/frontend.log" 2>&1 &
FE_PID=$!

for i in {1..15}; do
    sleep 0.5
    if curl -sf http://localhost:5173/ > /dev/null 2>&1; then
        ok "Frontend 就绪 (PID=$FE_PID)"
        break
    fi
    if [ $i -eq 15 ]; then
        warn "Frontend 启动较慢，查看日志: $LOG_DIR/frontend.log"
    fi
done

# ---- 4. WebSocket 测试 ----
echo "[4/4] WebSocket 连通性测试..."
python3 -c "
import asyncio, json, websockets
async def t():
    ws = await websockets.connect('ws://localhost:8000/ws')
    await ws.send(json.dumps({'type':'ping'}))
    r = json.loads(await ws.recv())
    if r['type'] != 'pong': raise Exception('unexpected response')
asyncio.run(t())
" 2>/dev/null && ok "WebSocket 连通正常" || warn "WebSocket 测试跳过 (pip install websockets)"

echo ""
echo -e "========================================"
echo -e "  ${GREEN}服务已全部启动${NC}"
echo ""
echo "  开发模式:"
echo "    前端:  http://localhost:5173"
echo "    API:   http://localhost:8000"
echo "    WS:    ws://localhost:8000/ws"
echo ""
echo "  生产模式 (Nginx 反向代理):"
echo "    nginx -c $PROJECT_DIR/nginx/nginx.conf"
echo "    统一入口: http://localhost:80"
if [ -n "$LAN_IP" ]; then
echo ""
echo "  局域网访问:"
echo "    http://$LAN_IP:5173"
fi
echo ""
echo "  项目结构:"
echo "    engine/   → C++ 对战引擎 (src/ include/ data/)"
echo "    gateway/  → Python API 网关 (api-server/)"
echo "    frontend/ → Vue.js SPA"
echo "    nginx/    → 反向代理 (生产模式)"
echo ""
echo "  日志: $LOG_DIR/"
echo "========================================"
