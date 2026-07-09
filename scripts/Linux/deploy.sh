#!/bin/bash
# ============================================================
#  PokemonSimulator — 一键部署 (单 IP 驱动)
#
#  前提: 集群网络已通 (SSH 免密或密码已就绪)
#
#  用法:
#    MASTER_IP=100.107.105.99 bash deploy.sh
#
#  可选覆盖:
#    MASTER_IP        集群 Master IP         (必需)
#    CLOUD_IP         云服务器对外 IP         (默认: 自动检测)
#    SSH_USER         VM SSH 用户名          (默认: hadoop)
#    APP_DIR          云服务器安装目录        (默认: /opt/pokemon-simulator)
#    WITH_DAEMON      云服务器上启动 daemon   (默认: yes)
# ============================================================
set -e

# ═══════════════════════ 输入 ═══════════════════════
MASTER_IP="${MASTER_IP:?请设置 MASTER_IP 环境变量，例: MASTER_IP=100.107.105.99 bash deploy.sh}"
CLOUD_IP="${CLOUD_IP:-$(curl -s ifconfig.me 2>/dev/null || hostname -I | awk '{print $1}')}"
SSH_USER="${SSH_USER:-hadoop}"
APP_DIR="${APP_DIR:-/opt/pokemon-simulator}"
WITH_DAEMON="${WITH_DAEMON:-yes}"

# 派生 — 不需要额外配置
KAFKA_BROKER="${MASTER_IP}:9092"
STATS_BACKEND="http://${MASTER_IP}:8080"
REPO_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
ok()   { echo -e "  ${GREEN}✅${NC} $1"; }
warn() { echo -e "  ${YELLOW}⚠️${NC}  $1"; }
die()  { echo -e "  ${RED}❌${NC} $1"; exit 1; }
info() { echo -e "  ${CYAN}ℹ${NC}  $1"; }
step() { echo -e "\n${YELLOW}═══ [$1]${NC} ${CYAN}$2${NC}"; }

echo ""
echo "  ██████╗  ██████╗ ██╗  ██╗███████╗███╗   ███╗ ██████╗ ███╗   ██╗"
echo "  ██╔══██╗██╔═══██╗██║ ██╔╝██╔════╝████╗ ████║██╔═══██╗████╗  ██║"
echo "  ██████╔╝██║   ██║█████╔╝ █████╗  ██╔████╔██║██║   ██║██╔██╗ ██║"
echo "  ██╔═══╝ ██║   ██║██╔═██╗ ██╔══╝  ██║╚██╔╝██║██║   ██║██║╚██╗██║"
echo "  ██║     ╚██████╔╝██║  ██╗███████╗██║ ╚═╝ ██║╚██████╔╝██║ ╚████║"
echo "  ╚═╝      ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝     ╚═╝ ╚═════╝ ╚═╝  ╚═══╝"
echo ""
echo "  Master: $MASTER_IP  |  Cloud: $CLOUD_IP  |  User: $SSH_USER"
echo ""

# ═══════════════════════════════════════
# 0. 连通性预检
# ═══════════════════════════════════════
step "0" "Preflight"

# 检测是否为 Ubuntu
if [ -f /etc/os-release ]; then
    . /etc/os-release
    [ "$ID" = "ubuntu" ] || warn "This server is $ID (expected ubuntu) — install may vary"
fi

# 检测 SSH 到 Master
if ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no -o BatchMode=yes \
    "${SSH_USER}@${MASTER_IP}" "hostname" 2>/dev/null; then
    ok "SSH to $MASTER_IP: OK"
else
    die "Cannot SSH to $SSH_USER@$MASTER_IP — check keys or password"
fi

# 检测 Master OS
MASTER_OS=$(ssh "${SSH_USER}@${MASTER_IP}" "grep '^ID=' /etc/os-release 2>/dev/null | cut -d= -f2" 2>/dev/null || echo "unknown")
[ "$MASTER_OS" = "ubuntu" ] || warn "Master is $MASTER_OS (expected ubuntu) — VM setup may vary"

# ═══════════════════════════════════════
# 1. Cloud Server 依赖
# ═══════════════════════════════════════
step "1/6" "Cloud: system dependencies"

sudo apt-get update -qq 2>/dev/null || true
for pkg in python3 python3-pip python3-venv nginx sqlite3 curl git; do
    dpkg -l "$pkg" &>/dev/null 2>&1 || {
        info "Installing $pkg..."
        sudo apt-get install -y -qq "$pkg" 2>/dev/null || die "Cannot install $pkg"
    }
done

# Node.js
if ! node -v 2>/dev/null | grep -q "v20"; then
    info "Installing Node.js 20.x..."
    curl -fsSL https://deb.nodesource.com/setup_20.x | sudo -E bash - 2>/dev/null
    sudo apt-get install -y -qq nodejs 2>/dev/null || die "Node.js install failed"
fi
ok "Node $(node -v) | Python $(python3 --version) | Nginx $(nginx -v 2>&1)"

# ═══════════════════════════════════════
# 2. Cloud Server 应用部署
# ═══════════════════════════════════════
step "2/6" "Cloud: application"

sudo mkdir -p "$APP_DIR" /var/log/pokemon-simulator
sudo chown -R "$USER:$USER" "$APP_DIR" /var/log/pokemon-simulator

[ "$REPO_DIR" != "$APP_DIR" ] && {
    info "Syncing code..."
    rsync -a --delete \
          --exclude='.git' --exclude='node_modules' --exclude='venv' \
          --exclude='__pycache__' --exclude='*.bak' --exclude='logs' \
          --exclude='frontend/dist' "$REPO_DIR/" "$APP_DIR/"
}

# Python venv
python3 -m venv "$APP_DIR/venv"
source "$APP_DIR/venv/bin/activate"
pip install -q --upgrade pip
pip install -q fastapi "uvicorn[standard]" httpx websockets python-multipart aiofiles kafka-python
ok "Python deps"

# Frontend
cd "$APP_DIR/frontend"
npm install --no-audit --no-fund 2>&1 | tail -1
npx vite build 2>&1 | tail -3
[ -f dist/index.html ] && ok "Frontend built" || die "Frontend build failed"

# Runtime config
mkdir -p "$APP_DIR/conf"
cat > "$APP_DIR/conf/api.env" << EOF
KAFKA_BROKER=$KAFKA_BROKER
STATS_BACKEND=$STATS_BACKEND
API_HOST=0.0.0.0
API_PORT=9000
EOF

# Nginx
sudo tee /etc/nginx/sites-available/pokemon > /dev/null << NGINXEOF
server {
    listen 80;
    server_name $CLOUD_IP;
    root $APP_DIR/frontend/dist;
    index index.html;
    location /api/v1/stats/ { proxy_pass $STATS_BACKEND; proxy_http_version 1.1; proxy_set_header Host \$host; }
    location /api/ { proxy_pass http://127.0.0.1:9000; proxy_http_version 1.1; proxy_set_header Host \$host; }
    location /ws  { proxy_pass http://127.0.0.1:9000; proxy_http_version 1.1; proxy_set_header Upgrade \$http_upgrade; proxy_set_header Connection "Upgrade"; }
    location /     { try_files \$uri \$uri/ /index.html; }
}
NGINXEOF
sudo ln -sf /etc/nginx/sites-available/pokemon /etc/nginx/sites-enabled/ 2>/dev/null || true
sudo rm -f /etc/nginx/sites-enabled/default 2>/dev/null || true
sudo nginx -t && sudo systemctl reload nginx
ok "Nginx configured"

# Systemd
sudo tee /etc/systemd/system/pokemon-api.service > /dev/null << SVCEOF
[Unit]
Description=PokemonSimulator API
After=network.target
[Service]
Type=simple
User=$USER
WorkingDirectory=$APP_DIR
EnvironmentFile=$APP_DIR/conf/api.env
ExecStart=$APP_DIR/venv/bin/python api-server/standalone_server.py
Restart=always
RestartSec=5
[Install]
WantedBy=multi-user.target
SVCEOF

sudo systemctl daemon-reload
sudo systemctl enable pokemon-api
sudo systemctl restart pokemon-api
sleep 3
curl -s http://localhost:9000/api/v1/health | grep -q '"ok"' && ok "API running" || die "API failed"
ok "Cloud server ready"

# ═══════════════════════════════════════
# 3. Master VM 依赖安装
# ═══════════════════════════════════════
step "3/6" "VM: system dependencies"

ssh "${SSH_USER}@${MASTER_IP}" bash -s << 'VMSETUP'
set -e
sudo apt-get update -qq 2>/dev/null || true
for pkg in python3 python3-pip sqlite3 openjdk-11-jdk curl; do
    dpkg -l "$pkg" &>/dev/null 2>&1 || sudo apt-get install -y -qq "$pkg" 2>/dev/null || echo "WARN: cannot install $pkg"
done
echo "VM system deps installed"
VMSETUP

# Python deps on VM
ssh "${SSH_USER}@${MASTER_IP}" "pip3 install -q kafka-python 2>/dev/null || pip3 install --user -q kafka-python 2>/dev/null || echo 'WARN: pip install failed'"
ok "VM dependencies"

# ═══════════════════════════════════════
# 4. Master VM Kafka 安装 (如未安装)
# ═══════════════════════════════════════
step "4/6" "VM: Kafka"

ssh "${SSH_USER}@${MASTER_IP}" bash -s << KAFKASETUP
set -e
if [ -d ~/kafka ] && [ -f ~/kafka/bin/kafka-server-start.sh ]; then
    echo "Kafka already installed"
    exit 0
fi
K_VERSION=3.9.0
K_SCALA=2.13
echo "Downloading Kafka \$K_VERSION..."
cd ~
curl -sO "https://dlcdn.apache.org/kafka/\$K_VERSION/kafka_\${K_SCALA}-\$K_VERSION.tgz"
tar xzf kafka_\${K_SCALA}-\$K_VERSION.tgz
mv kafka_\${K_SCALA}-\$K_VERSION kafka
rm kafka_\${K_SCALA}-\$K_VERSION.tgz

# KRaft config
CID=\$(~/kafka/bin/kafka-storage.sh random-uuid)
~/kafka/bin/kafka-storage.sh format -t \$CID -c ~/kafka/config/kraft/server.properties 2>/dev/null
sed -i 's/^#\?listeners=.*/listeners=PLAINTEXT:\/\/0.0.0.0:9092/' ~/kafka/config/kraft/server.properties
sed -i 's/^#\?advertised.listeners=.*/advertised.listeners=PLAINTEXT:\/\/${MASTER_IP}:9092/' ~/kafka/config/kraft/server.properties
echo "Kafka installed"
KAFKASETUP

# Start Kafka
ssh "${SSH_USER}@${MASTER_IP}" bash -s << 'STARTKAFKA'
if ! pgrep -f "kafka.Kafka" >/dev/null 2>&1; then
    ~/kafka/bin/kafka-server-start.sh -daemon ~/kafka/config/kraft/server.properties
    sleep 5
fi
pgrep -f "kafka.Kafka" >/dev/null 2>&1 && echo "Kafka running" || echo "WARN: Kafka not running"
STARTKAFKA

# Create topics
ssh "${SSH_USER}@${MASTER_IP}" bash -s << 'TOPICS'
for topic in battle.logs player.ui.events; do
    ~/kafka/bin/kafka-topics.sh --bootstrap-server localhost:9092 --topic "$topic" \
        --create --partitions 3 --replication-factor 1 --if-not-exists 2>/dev/null || true
done
~/kafka/bin/kafka-topics.sh --list --bootstrap-server localhost:9092
TOPICS
ok "Kafka ready ($MASTER_IP:9092)"

# ═══════════════════════════════════════
# 5. Master VM  桥 + Stats 部署
# ═══════════════════════════════════════
step "5/6" "VM: Bridge + Stats"

# Deploy scripts to VM
scp "$REPO_DIR/scripts/kafka_to_sqlite.py" "${SSH_USER}@${MASTER_IP}:~/" 2>/dev/null
scp "$REPO_DIR/scripts/stats_server.py"     "${SSH_USER}@${MASTER_IP}:~/" 2>/dev/null

# Start bridge
ssh "${SSH_USER}@${MASTER_IP}" bash -s << STARTSVC
mkdir -p ~/logs
for svc in kafka_to_sqlite stats_server; do
    if ! pgrep -f "\${svc}.py" >/dev/null 2>&1; then
        nohup python3 ~/\${svc}.py > ~/logs/\${svc}.log 2>&1 &
        sleep 2
    fi
    pgrep -f "\${svc}.py" >/dev/null 2>&1 && echo "\$svc: OK" || echo "WARN: \$svc not running"
done
STARTSVC

# Verify data endpoint
sleep 2
curl -s --connect-timeout 5 "$STATS_BACKEND/api/v1/stats/deep/summary" | grep -q '"ok"' && \
    ok "Stats API responding" || warn "Stats API not reachable (may need firewall rule)"

# ═══════════════════════════════════════
# 6. Cloud  启动 Data Daemon
# ═══════════════════════════════════════
step "6/6" "Cloud: Data Daemon"

if [ "$WITH_DAEMON" = "yes" ]; then
    cd "$APP_DIR"
    npm install --production --no-audit --no-fund 2>&1 | tail -1
    [ -d engine-adapter ] && (cd engine-adapter && npm install --production --no-audit --no-fund 2>&1 | tail -1)

    NODE_BIN=$(which node)
    sudo tee /etc/systemd/system/pokemon-daemon.service > /dev/null << SVCEOF
[Unit]
Description=Pokemon Data Daemon
After=network.target pokemon-api.service
[Service]
Type=simple
User=$USER
WorkingDirectory=$APP_DIR
Environment=KAFKA_BROKER=$KAFKA_BROKER
ExecStart=$NODE_BIN engine-adapter/data_daemon.js
Restart=always
RestartSec=10
StandardOutput=append:/var/log/pokemon-simulator/daemon.log
StandardError=append:/var/log/pokemon-simulator/daemon.log
[Install]
WantedBy=multi-user.target
SVCEOF
    sudo systemctl daemon-reload
    sudo systemctl enable pokemon-daemon
    sudo systemctl start pokemon-daemon
    sleep 5
    systemctl is-active --quiet pokemon-daemon && ok "Daemon running" || warn "Daemon failed"
fi

# ═══════════════════════════════════════
echo ""
echo "============================================"
echo "  ${GREEN}Deploy Complete${NC}"
echo "============================================"
echo "  Frontend:       http://$CLOUD_IP"
echo "  API Health:     http://$CLOUD_IP/api/v1/health"
echo "  Real-time:      http://$CLOUD_IP/realtime"
echo "  Analytics:      http://$CLOUD_IP/analytics"
echo "  Stats Dashboard:http://$CLOUD_IP/stats"
echo ""
echo "  Master VM:      $SSH_USER@$MASTER_IP"
echo "  Kafka:          $KAFKA_BROKER"
echo "  Stats API:      $STATS_BACKEND"
echo ""
echo "  Manage Cloud:"
echo "    sudo systemctl restart pokemon-api"
echo "    sudo systemctl restart pokemon-daemon"
echo "    sudo journalctl -u pokemon-api -f"
echo ""
echo "  Manage VM:"
echo "    ssh $SSH_USER@$MASTER_IP"
echo "    pgrep -f kafka_to_sqlite && pgrep -f stats_server"
echo "============================================"
