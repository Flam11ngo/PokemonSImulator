#!/bin/bash
# ============================================
#  PokemonSimulator — Production Deploy
#  Target: 115.28.210.95 (Linux)
#  No Kafka / Spark / real-time data pipeline
# ============================================
set -e

APP_DIR=/opt/pokemon-simulator
VENV_DIR=$APP_DIR/venv
LOG_DIR=/var/log/pokemon-simulator
NGINX_CONF=/etc/nginx/sites-enabled/pokemon-simulator
SERVER_IP="115.28.210.95"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()  { echo -e "  ${GREEN}✅ $1${NC}"; }
step() { echo -e "\n${YELLOW}[$1]${NC} $2"; }

echo "============================================"
echo "  PokemonSimulator Deploy"
echo "  Target: $SERVER_IP"
echo "============================================"

# ── 1. System dependencies ──
step "1/6" "Installing system packages..."
sudo apt-get update -qq
sudo apt-get install -y -qq python3 python3-pip python3-venv nodejs npm nginx sqlite3 curl 2>/dev/null
ok "System packages"

# ── 2. App directory ──
step "2/6" "Setting up app directory..."
sudo mkdir -p $APP_DIR $LOG_DIR
sudo chown -R $USER:$USER $APP_DIR $LOG_DIR

# Copy project files (assumes repo is extracted to $APP_DIR)
cd $APP_DIR
ok "App directory: $APP_DIR"

# ── 3. Python venv + deps ──
step "3/6" "Installing Python dependencies..."
python3 -m venv $VENV_DIR
source $VENV_DIR/bin/activate
pip install -q fastapi uvicorn httpx websockets python-multipart aiofiles
ok "Python deps installed"

# ── 4. Node deps ──
step "4/6" "Installing Node dependencies..."
cd $APP_DIR/engine-adapter
npm install --production pokemon-showdown 2>/dev/null || true
ok "Node deps installed"

# ── 5. Nginx config ──
step "5/6" "Configuring Nginx..."
sudo tee $NGINX_CONF > /dev/null << NGINXEOF
server {
    listen 80;
    server_name $SERVER_IP;

    # Frontend static files
    root $APP_DIR/frontend/dist;
    index index.html;

    # API + WebSocket proxy
    location /api/ {
        proxy_pass http://127.0.0.1:9000;
        proxy_http_version 1.1;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
    }
    location /ws {
        proxy_pass http://127.0.0.1:9000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade \$http_upgrade;
        proxy_set_header Connection "Upgrade";
    }

    # SPA fallback
    location / {
        try_files \$uri \$uri/ /index.html;
    }
}
NGINXEOF
sudo ln -sf $NGINX_CONF /etc/nginx/sites-enabled/pokemon-simulator 2>/dev/null || true
sudo rm -f /etc/nginx/sites-enabled/default
sudo nginx -t && sudo systemctl reload nginx
ok "Nginx configured"

# ── 6. Systemd services ──
step "6/6" "Creating systemd services..."

sudo tee /etc/systemd/system/pokemon-api.service > /dev/null << SVCEOF
[Unit]
Description=PokemonSimulator API Server
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=$APP_DIR
Environment="PATH=$VENV_DIR/bin:/usr/bin"
ExecStart=$VENV_DIR/bin/python api-server/standalone_server.py
Restart=always
RestartSec=5
StandardOutput=append:$LOG_DIR/api.log
StandardError=append:$LOG_DIR/api.log

[Install]
WantedBy=multi-user.target
SVCEOF

sudo systemctl daemon-reload
sudo systemctl enable pokemon-api
sudo systemctl restart pokemon-api
sleep 3

if curl -s http://localhost:9000/api/v1/health | grep -q ok; then
    ok "API service running"
else
    echo -e "  ${RED}❌ API service failed — check: journalctl -u pokemon-api${NC}"
fi

# ── Done ──
echo ""
echo "============================================"
echo "  Deploy Complete"
echo "============================================"
echo ""
echo "  Frontend:  http://$SERVER_IP"
echo "  API:       http://$SERVER_IP/api/v1/health"
echo "  WS:        ws://$SERVER_IP/ws"
echo ""
echo "  Manage:"
echo "    sudo systemctl restart pokemon-api"
echo "    sudo journalctl -u pokemon-api -f"
echo "    sudo nginx -t && sudo systemctl reload nginx"
echo "============================================"
