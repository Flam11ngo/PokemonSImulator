#!/bin/bash
# ============================================================
# Redis 集群安装脚本 (Person C)
# Master + 2 Slave 物理机部署
#
# 用法:
#   Node1 (Master):         ./install-redis.sh master 100.107.105.99
#   Node2 (Slave):          ./install-redis.sh slave <node2-tailscale-ip> 100.107.105.99
#   Node3 (Slave):          ./install-redis.sh slave <node3-tailscale-ip> 100.107.105.99
# ============================================================
set -euo pipefail

REDIS_VERSION="7.2.5"
REDIS_HOME="/opt/redis"
REDIS_DATA="/data/redis"

ROLE="${1:-}"
NODE_IP="${2:-}"
MASTER_IP="${3:-100.107.105.99}"

if [ -z "$ROLE" ] || [ -z "$NODE_IP" ]; then
    echo "用法: $0 <master|slave> <本机Tailscale_IP> [Master_IP]"
    exit 1
fi

echo "=== 安装 Redis $REDIS_VERSION ($ROLE) ==="
echo "本机 IP: $NODE_IP"
echo "Master IP: $MASTER_IP"

# ---------- 1. 安装 Redis ----------
echo "[1/4] 安装 Redis..."
if ! command -v redis-server &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq build-essential tcl
    # 从源码编译
    if [ ! -f "/tmp/redis-${REDIS_VERSION}.tar.gz" ]; then
        wget -q --show-progress \
            "https://download.redis.io/releases/redis-${REDIS_VERSION}.tar.gz" \
            -O "/tmp/redis-${REDIS_VERSION}.tar.gz"
    fi
    tar -xzf "/tmp/redis-${REDIS_VERSION}.tar.gz" -C /tmp/
    cd "/tmp/redis-${REDIS_VERSION}"
    make -j$(nproc) > /dev/null 2>&1
    sudo make install > /dev/null 2>&1
    cd -
else
    echo "Redis 已安装: $(redis-server --version)"
fi

# ---------- 2. 创建目录 ----------
echo "[2/4] 创建目录..."
sudo mkdir -p "$REDIS_DATA"
sudo chown -R "$USER:$USER" "$REDIS_DATA"

# ---------- 3. 配置 ----------
echo "[3/4] 配置 Redis..."

REDIS_CONF="/etc/redis/pokemon-simulator.conf"

sudo tee "$REDIS_CONF" > /dev/null << REDISCONF
# === PokemonSimulator Redis ($ROLE) ===

# 网络
bind 0.0.0.0
port 6379
protected-mode yes
tcp-backlog 511
timeout 300
tcp-keepalive 300

# 数据持久化
dir ${REDIS_DATA}
dbfilename dump.rdb
save 900 1
save 300 10
save 60 10000
stop-writes-on-bgsave-error yes
rdbcompression yes

# AOF
appendonly yes
appendfilename "appendonly.aof"
appendfsync everysec
auto-aof-rewrite-percentage 100
auto-aof-rewrite-min-size 64mb

# 内存
maxmemory 512mb
maxmemory-policy allkeys-lru

# 日志
loglevel notice
logfile /var/log/redis-pokemon.log

# 慢查询
slowlog-log-slower-than 10000
slowlog-max-len 128
REDISCONF

if [ "$ROLE" = "slave" ]; then
    # Slave 配置: 复制 Master
    echo "" | sudo tee -a "$REDIS_CONF"
    echo "# 主从复制" | sudo tee -a "$REDIS_CONF"
    echo "replicaof ${MASTER_IP} 6379" | sudo tee -a "$REDIS_CONF"
    echo "replica-read-only yes" | sudo tee -a "$REDIS_CONF"
    echo "replica-serve-stale-data yes" | sudo tee -a "$REDIS_CONF"
fi

# ---------- 4. 启动 ----------
echo "[4/4] 启动 Redis..."

sudo tee /etc/systemd/system/redis-pokemon.service > /dev/null << SYSTEMD
[Unit]
Description=Redis (PokemonSimulator)
After=network.target

[Service]
Type=notify
User=$USER
ExecStart=/usr/local/bin/redis-server ${REDIS_CONF} --supervised systemd
ExecStop=/usr/local/bin/redis-cli shutdown
Restart=on-failure
RestartSec=5
LimitNOFILE=65536

[Install]
WantedBy=multi-user.target
SYSTEMD

sudo systemctl daemon-reload
sudo systemctl enable redis-pokemon
sudo systemctl start redis-pokemon

echo ""
echo "=== Redis $ROLE 已启动 ==="
echo "地址: ${NODE_IP}:6379"
echo ""
echo "检查状态:"
echo "  redis-cli -h ${NODE_IP} ping"
echo "  redis-cli -h ${NODE_IP} info replication"
if [ "$ROLE" = "master" ]; then
    echo ""
    echo "测试写入:"
    echo "  redis-cli -h ${NODE_IP} SET test 'hello pokemon'"
    echo "  redis-cli -h ${NODE_IP} GET test"
fi
echo ""
echo "=== 安装完成 ==="
