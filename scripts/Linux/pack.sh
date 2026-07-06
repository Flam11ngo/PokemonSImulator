#!/usr/bin/env bash
# =============================================================================
# pack.sh — 最小部署打包脚本 (开发机上运行)
# =============================================================================
# 用法:
#   ./scripts/pack.sh              # 构建 + 打包
#   ./scripts/pack.sh --skip-build # 跳过构建, 仅打包 (已有二进制)
# =============================================================================
set -euo pipefail

GREEN='\033[0;32m'; CYAN='\033[0;36m'; NC='\033[0m'
log() { echo -e "${GREEN}[pack]${NC} $*"; }

SKIP_BUILD=false
[[ $# -gt 0 && "$1" == "--skip-build" ]] && SKIP_BUILD=true

cd "$(dirname "$0")/.."
ROOT=$(pwd)
OUT="$ROOT/deploy.tar.gz"

# ---------------------------------------------------------------------------
# 1. 构建 C++ 引擎
# ---------------------------------------------------------------------------
if $SKIP_BUILD; then
    if [[ ! -f build/PokemonSimulator ]]; then
        echo "错误: build/PokemonSimulator 不存在, 请先编译 !"
        exit 1
    fi
    log "跳过 C++ 构建 (已有二进制)"
else
    log "构建 C++ 引擎 (Release, 无测试)..."
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
    cmake --build build --target PokemonSimulator
    log "C++ 引擎构建完成"
fi

# ---------------------------------------------------------------------------
# 2. 构建前端
# ---------------------------------------------------------------------------
if [[ -d frontend/dist ]] && [[ -n "$(ls frontend/dist/ 2>/dev/null)" ]]; then
    log "前端已有 dist/ 目录"
else
    log "构建前端..."
    cd frontend
    npm install --no-audit --no-fund --silent
    npm run build
    cd "$ROOT"
    log "前端构建完成"
fi

# ---------------------------------------------------------------------------
# 3. 迁移 JSON → SQLite (standalone_server.py 依赖)
# ---------------------------------------------------------------------------
if [[ ! -f data/pokemon.db ]]; then
    log "迁移 JSON → SQLite..."
    python3 scripts/migrate_json_to_sqlite.py
    log "pokemon.db 生成完成"
else
    log "pokemon.db 已存在"
fi

# ---------------------------------------------------------------------------
# 4. 打包
# ---------------------------------------------------------------------------
log "打包到 $OUT ..."

TMP=$(mktemp -d)
mkdir -p "$TMP/pokemon-sim/bin" "$TMP/pokemon-sim/data" "$TMP/pokemon-sim/frontend"

# C++ 引擎
cp build/PokemonSimulator "$TMP/pokemon-sim/bin/"
chmod +x "$TMP/pokemon-sim/bin/PokemonSimulator"

# 游戏数据 (C++ 引擎需要 JSON, Python 需要 pokemon.db)
for f in pokemon.db species.json moves.json abilities.json items.json learnsets.json; do
    if [[ -f "data/$f" ]]; then
        cp "data/$f" "$TMP/pokemon-sim/data/"
    fi
done

# 前端静态文件
cp -r frontend/dist/* "$TMP/pokemon-sim/frontend/"

# Python 服务 — 复制并修正路径 (PROJECT_ROOT + ENGINE_BIN)
sed -e 's|PROJECT_ROOT = Path(__file__).resolve().parent.parent|PROJECT_ROOT = Path(__file__).resolve().parent|' \
    -e 's|ENGINE_BIN = PROJECT_ROOT / "build" / "PokemonSimulator"|ENGINE_BIN = PROJECT_ROOT / "bin" / "PokemonSimulator"|' \
    api-server/standalone_server.py > "$TMP/pokemon-sim/standalone_server.py"

# server.py: wrapper 负责挂载静态文件 + 启动
cat > "$TMP/pokemon-sim/server.py" << 'SERVER_PY'
#!/usr/bin/env python3
"""Minimal production launcher: serves frontend + WebSocket on port 8000."""
import os, sys
os.chdir(os.path.dirname(os.path.abspath(__file__)))

from standalone_server import app
from fastapi.staticfiles import StaticFiles

# 挂载前端静态文件 (SPA fallback)
app.mount("/", StaticFiles(directory="frontend", html=True), name="frontend")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
SERVER_PY

# 需求文件 (standalone_server.py 最小依赖)
cat > "$TMP/pokemon-sim/requirements.txt" << 'EOF'
fastapi>=0.100.0
uvicorn[standard]>=0.30.0
aiofiles>=23.0.0
EOF

# 生成安装脚本 (服务器端运行)
cat > "$TMP/pokemon-sim/install.sh" << 'INSTALL_EOF'
#!/usr/bin/env bash
# =============================================================================
# PokemonSimulator 最小部署 — 服务器端安装脚本
# 用法: sudo bash install.sh
# =============================================================================
set -euo pipefail
GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
log()  { echo -e "${GREEN}[install]${NC} $*"; }
err()  { echo -e "${RED}[ERROR]${NC} $*"; }

INSTALL_DIR="/opt/pokemon-sim"
SRC="$(cd "$(dirname "$0")" && pwd)"

# ---- 1. 系统依赖 ----
log "安装系统依赖..."
if command -v apt-get &>/dev/null; then
    apt-get update -qq
    # python3 + C++ 运行时库 (PokemonSimulator 需要 libcurl4)
    apt-get install -y -qq python3 python3-pip python3-venv libcurl4
elif command -v yum &>/dev/null; then
    yum install -y python3 python3-pip libcurl
elif command -v dnf &>/dev/null; then
    dnf install -y python3 python3-pip libcurl
else
    err "未检测到支持的包管理器 (apt/yum/dnf)"
    exit 1
fi

# ---- 2. 创建运行用户 ----
if ! id -u pokemon &>/dev/null; then
    useradd -r -m -s /bin/bash pokemon 2>/dev/null || useradd -m -s /bin/bash pokemon
    log "创建用户 pokemon"
fi

# ---- 3. 安装到目标目录 ----
if [[ "$SRC" != "$INSTALL_DIR" ]]; then
    log "安装到 ${INSTALL_DIR}..."
    cp -r "$SRC" "$INSTALL_DIR"
fi
chown -R pokemon:pokemon "$INSTALL_DIR"

# ---- 4. Python 虚拟环境 ----
log "创建 Python 虚拟环境..."
cd "$INSTALL_DIR"
python3 -m venv venv
./venv/bin/pip install --no-cache-dir -r requirements.txt

# ---- 5. systemd 服务 ----
log "注册 systemd 服务..."
cat > /etc/systemd/system/pokemon-sim.service << SYSTEMD_EOF
[Unit]
Description=PokemonSimulator Server (WebSocket + Frontend)
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=pokemon
WorkingDirectory=${INSTALL_DIR}
ExecStart=${INSTALL_DIR}/venv/bin/python server.py
Restart=always
RestartSec=3
StandardOutput=journal
StandardError=journal
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
SYSTEMD_EOF

systemctl daemon-reload
systemctl enable pokemon-sim
systemctl restart pokemon-sim

# ---- 6. 验证 ----
sleep 2
if systemctl is-active --quiet pokemon-sim; then
    IP=$(hostname -I 2>/dev/null | awk '{print $1}')
    [[ -z "$IP" ]] && IP="<服务器IP>"
    log "=============================================="
    log "  ✓ 部署成功!"
    log ""
    log "  前端:       http://${IP}:8000"
    log "  WebSocket:  ws://${IP}:8000/ws"
    log "  健康检查:   http://${IP}:8000/api/v1/health"
    log ""
    log "  常用命令:"
    log "    systemctl status pokemon-sim"
    log "    journalctl -u pokemon-sim -f"
    log "    systemctl restart pokemon-sim"
    log "=============================================="
else
    err "=============================================="
    err "  服务启动失败!"
    err "  查看日志: journalctl -u pokemon-sim -n 50"
    err "=============================================="
    exit 1
fi
INSTALL_EOF

chmod +x "$TMP/pokemon-sim/install.sh"

# 生成 tarball
cd "$TMP"
tar czf "$OUT" pokemon-sim/
rm -rf "$TMP"

# ---------------------------------------------------------------------------
# 5. 完成
# ---------------------------------------------------------------------------
SIZE=$(du -h "$OUT" | cut -f1)
echo ""
log "=============================================="
log "  打包完成!"
log "  文件: $(basename "$OUT") (${SIZE})"
log ""
log "部署到服务器 (3 步):"
echo ""
echo -e "  ${CYAN}# 1. 上传到服务器${NC}"
echo -e "  ${CYAN}scp ${OUT} root@<服务器IP>:/tmp/${NC}"
echo ""
echo -e "  ${CYAN}# 2. SSH 到服务器${NC}"
echo -e "  ${CYAN}ssh root@<服务器IP>${NC}"
echo ""
echo -e "  ${CYAN}# 3. 解压并安装${NC}"
echo -e "  ${CYAN}cd /tmp && tar xzf $(basename "$OUT") && cd pokemon-sim && sudo bash install.sh${NC}"
log "=============================================="
