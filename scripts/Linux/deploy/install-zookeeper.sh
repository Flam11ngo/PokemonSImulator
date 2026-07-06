#!/bin/bash
# ============================================================
# Apache ZooKeeper 集群安装脚本 (Person C)
# 3 节点 Ensemble
#
# 用法（每台节点都执行）:
#   Node1: ./install-zookeeper.sh 1 100.107.105.99 "<node2-ip>:2888:3888,<node3-ip>:2888:3888"
#   Node2: ./install-zookeeper.sh 2 <node2-tailscale-ip> "<node1-ip>:2888:3888,<node3-ip>:2888:3888"
#   Node3: ./install-zookeeper.sh 3 <node3-tailscale-ip> "<node1-ip>:2888:3888,<node2-ip>:2888:3888"
# ============================================================
set -euo pipefail

ZK_VERSION="3.8.4"
ZK_HOME="/opt/zookeeper"
ZK_DATA="/data/zookeeper"

MYID="${1:-}"
NODE_IP="${2:-}"

if [ -z "$MYID" ] || [ -z "$NODE_IP" ]; then
    echo "用法: $0 <myid: 1|2|3> <本机Tailscale_IP>"
    echo "注意: zoo.cfg 中的集群列表需要手动配置"
    exit 1
fi

echo "=== 安装 ZooKeeper $ZK_VERSION (Node $MYID) ==="
echo "本机 IP: $NODE_IP"

# ---------- 1. 检查 Java ----------
echo "[1/5] 检查 Java..."
if ! command -v java &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq openjdk-11-jdk
fi

# ---------- 2. 创建目录 ----------
echo "[2/5] 创建目录..."
sudo mkdir -p "$ZK_HOME" "$ZK_DATA"
sudo chown -R "$USER:$USER" "$ZK_HOME" "$ZK_DATA"

# ---------- 3. 下载 ZooKeeper ----------
echo "[3/5] 下载 ZooKeeper $ZK_VERSION..."
if [ ! -f "/tmp/apache-zookeeper-${ZK_VERSION}-bin.tar.gz" ]; then
    wget -q --show-progress \
        "https://archive.apache.org/dist/zookeeper/zookeeper-${ZK_VERSION}/apache-zookeeper-${ZK_VERSION}-bin.tar.gz" \
        -O "/tmp/apache-zookeeper-${ZK_VERSION}-bin.tar.gz"
fi

if [ ! -d "$ZK_HOME/bin" ]; then
    tar -xzf "/tmp/apache-zookeeper-${ZK_VERSION}-bin.tar.gz" -C /tmp/
    mv /tmp/apache-zookeeper-${ZK_VERSION}-bin/* "$ZK_HOME/"
    rm -rf /tmp/apache-zookeeper-${ZK_VERSION}-bin
fi

# ---------- 4. 配置 ----------
echo "[4/5] 配置 ZooKeeper..."

cat > "$ZK_HOME/conf/zoo.cfg" << ZKCFG
# === PokemonSimulator ZooKeeper Ensemble ===
tickTime=2000
initLimit=10
syncLimit=5
dataDir=${ZK_DATA}
clientPort=2181
clientPortAddress=0.0.0.0

# 集群节点 (需根据实际 IP 填写)
# 格式: server.X=host:peerPort:leaderElectionPort
# === TODO: 填写所有 3 个节点的 Tailscale IP ===
server.1=100.107.105.99:2888:3888
server.2=<node2-tailscale-ip>:2888:3888
server.3=<node3-tailscale-ip>:2888:3888

# 性能调优
maxClientCnxns=60
autopurge.snapRetainCount=3
autopurge.purgeInterval=24
ZKCFG

# 设置 myid
echo "$MYID" > "$ZK_DATA/myid"

# 环境变量
if ! grep -q "ZK_HOME" ~/.bashrc; then
    cat >> ~/.bashrc << 'EOF'
# ZooKeeper
export ZK_HOME=/opt/zookeeper
export PATH=$PATH:$ZK_HOME/bin
EOF
fi
source ~/.bashrc 2>/dev/null || true

# ---------- 5. 启动 ----------
echo "[5/5] 启动 ZooKeeper..."

sudo tee /etc/systemd/system/zookeeper.service > /dev/null << SYSTEMD
[Unit]
Description=Apache ZooKeeper
After=network.target

[Service]
Type=simple
User=$USER
ExecStart=${ZK_HOME}/bin/zkServer.sh start-foreground
ExecStop=${ZK_HOME}/bin/zkServer.sh stop
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
SYSTEMD

sudo systemctl daemon-reload
sudo systemctl enable zookeeper
sudo systemctl start zookeeper

echo ""
echo "=== ZooKeeper Node $MYID 已启动 ==="
echo "端口: ${NODE_IP}:2181"
echo ""
echo "检查状态: echo stat | nc localhost 2181"
echo "查看角色: echo srvr | nc localhost 2181"
echo ""
echo "⚠️  重要: 请编辑 $ZK_HOME/conf/zoo.cfg"
echo "    将 server.2 和 server.3 的 IP 替换为实际的 Tailscale IP"
echo ""
echo "=== 安装完成 ==="
