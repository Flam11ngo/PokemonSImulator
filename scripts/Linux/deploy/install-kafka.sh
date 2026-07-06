#!/bin/bash
# ============================================================
# Apache Kafka 集群安装脚本 (Person C)
# 3 Broker 物理机部署，ZooKeeper 协调
#
# 用法:
#   Node1: ./install-kafka.sh 1 100.107.105.99 "100.107.105.99:2181,<node2-ip>:2181,<node3-ip>:2181"
#   Node2: ./install-kafka.sh 2 <node2-tailscale-ip> "<zk列表>"
#   Node3: ./install-kafka.sh 3 <node3-tailscale-ip> "<zk列表>"
# ============================================================
set -euo pipefail

KAFKA_VERSION="3.6.2"
KAFKA_HOME="/opt/kafka"
KAFKA_DATA="/data/kafka"

BROKER_ID="${1:-}"
NODE_IP="${2:-}"
ZK_CONNECT="${3:-}"

if [ -z "$BROKER_ID" ] || [ -z "$NODE_IP" ] || [ -z "$ZK_CONNECT" ]; then
    echo "用法: $0 <broker_id: 1|2|3> <本机Tailscale_IP> <zk_connect_string>"
    echo "示例: $0 1 100.107.105.99 '100.107.105.99:2181,<ip2>:2181,<ip3>:2181'"
    exit 1
fi

echo "=== 安装 Kafka $KAFKA_VERSION (Broker $BROKER_ID) ==="
echo "本机 IP: $NODE_IP"
echo "ZK Connect: $ZK_CONNECT"

# ---------- 1. 检查 Java ----------
echo "[1/5] 检查 Java..."
if ! command -v java &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq openjdk-11-jdk
fi

# ---------- 2. 创建目录 ----------
echo "[2/5] 创建目录..."
sudo mkdir -p "$KAFKA_HOME" "$KAFKA_DATA"
sudo chown -R "$USER:$USER" "$KAFKA_HOME" "$KAFKA_DATA"

# ---------- 3. 下载 Kafka ----------
echo "[3/5] 下载 Kafka $KAFKA_VERSION..."
if [ ! -f "/tmp/kafka_2.13-${KAFKA_VERSION}.tgz" ]; then
    wget -q --show-progress \
        "https://archive.apache.org/dist/kafka/${KAFKA_VERSION}/kafka_2.13-${KAFKA_VERSION}.tgz" \
        -O "/tmp/kafka_2.13-${KAFKA_VERSION}.tgz"
fi

if [ ! -d "$KAFKA_HOME/bin" ]; then
    tar -xzf "/tmp/kafka_2.13-${KAFKA_VERSION}.tgz" -C /tmp/
    mv /tmp/kafka_2.13-${KAFKA_VERSION}/* "$KAFKA_HOME/"
    rm -rf /tmp/kafka_2.13-${KAFKA_VERSION}
fi

# ---------- 4. 配置 ----------
echo "[4/5] 配置 Kafka..."

cat > "$KAFKA_HOME/config/server.properties" << KAFKA
# === PokemonSimulator Kafka Broker ${BROKER_ID} ===
broker.id=${BROKER_ID}

# Listeners - 使用 Tailscale IP
listeners=PLAINTEXT://${NODE_IP}:9092
advertised.listeners=PLAINTEXT://${NODE_IP}:9092

# ZooKeeper
zookeeper.connect=${ZK_CONNECT}
zookeeper.connection.timeout.ms=18000

# Log storage
log.dirs=${KAFKA_DATA}
num.partitions=3
default.replication.factor=2
min.insync.replicas=1

# Retention
log.retention.hours=168
log.segment.bytes=1073741824
log.retention.check.interval.ms=300000

# Performance
num.network.threads=3
num.io.threads=8
socket.send.buffer.bytes=102400
socket.receive.buffer.bytes=102400
socket.request.max.bytes=104857600

# Topic auto-creation disabled (use explicit creation script)
auto.create.topics.enable=false

# Internal topics
offsets.topic.replication.factor=2
transaction.state.log.replication.factor=2
transaction.state.log.min.isr=1
KAFKA

# 环境变量
if ! grep -q "KAFKA_HOME" ~/.bashrc; then
    cat >> ~/.bashrc << 'EOF'
# Apache Kafka
export KAFKA_HOME=/opt/kafka
export PATH=$PATH:$KAFKA_HOME/bin
EOF
fi
source ~/.bashrc 2>/dev/null || true

# ---------- 5. 启动 ----------
echo "[5/5] 启动 Kafka Broker..."

# 创建 systemd 服务文件
sudo tee /etc/systemd/system/kafka.service > /dev/null << SYSTEMD
[Unit]
Description=Apache Kafka Broker
After=network.target

[Service]
Type=simple
User=$USER
ExecStart=${KAFKA_HOME}/bin/kafka-server-start.sh ${KAFKA_HOME}/config/server.properties
ExecStop=${KAFKA_HOME}/bin/kafka-server-stop.sh
Restart=on-failure
RestartSec=10

[Install]
WantedBy=multi-user.target
SYSTEMD

sudo systemctl daemon-reload
sudo systemctl enable kafka
sudo systemctl start kafka

echo ""
echo "=== Kafka Broker ${BROKER_ID} 已启动 ==="
echo "地址: ${NODE_IP}:9092"
echo ""
echo "检查状态: sudo systemctl status kafka"
echo "查看日志: sudo journalctl -u kafka -f"
echo ""
echo "=== 安装完成 ==="
