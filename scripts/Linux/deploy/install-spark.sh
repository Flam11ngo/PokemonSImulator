#!/bin/bash
# ============================================================
# Apache Spark 集群安装脚本 (Person B)
# Standalone 模式 + ZooKeeper HA
#
# 用法:
#   Node1 (Master): ./install-spark.sh master 100.107.105.99
#   Node2 (Worker): ./install-spark.sh worker <node2-tailscale-ip> 100.107.105.99
#   Node3 (Worker): ./install-spark.sh worker <node3-tailscale-ip> 100.107.105.99
# ============================================================
set -euo pipefail

SPARK_VERSION="3.5.4"
SPARK_HOME="/opt/spark"
SPARK_DATA="/data/spark"

ROLE="${1:-}"
NODE_IP="${2:-}"
MASTER_IP="${3:-100.107.105.99}"

# ZooKeeper 节点列表 (Tailscale IP)
ZK_NODES="${4:-${MASTER_IP}:2181}"

if [ -z "$ROLE" ] || [ -z "$NODE_IP" ]; then
    echo "用法: $0 <master|worker> <本机Tailscale_IP> [Master_IP] [ZK节点列表]"
    exit 1
fi

echo "=== 安装 Spark $SPARK_VERSION ($ROLE) ==="
echo "本机 IP: $NODE_IP"
echo "Master IP: $MASTER_IP"

# ---------- 1. 检查 Java ----------
echo "[1/5] 检查 Java..."
if ! command -v java &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq openjdk-11-jdk
fi

# ---------- 2. 创建目录 ----------
echo "[2/5] 创建目录..."
sudo mkdir -p "$SPARK_HOME" "$SPARK_DATA/work" "$SPARK_DATA/logs"
sudo chown -R "$USER:$USER" "$SPARK_HOME" "$SPARK_DATA"

# ---------- 3. 下载 Spark ----------
echo "[3/5] 下载 Spark $SPARK_VERSION..."
if [ ! -f "/tmp/spark-${SPARK_VERSION}-bin-hadoop3.tgz" ]; then
    wget -q --show-progress \
        "https://archive.apache.org/dist/spark/spark-${SPARK_VERSION}/spark-${SPARK_VERSION}-bin-hadoop3.tgz" \
        -O "/tmp/spark-${SPARK_VERSION}-bin-hadoop3.tgz"
fi

if [ ! -d "$SPARK_HOME/bin" ]; then
    tar -xzf "/tmp/spark-${SPARK_VERSION}-bin-hadoop3.tgz" -C /tmp/
    mv /tmp/spark-${SPARK_VERSION}-bin-hadoop3/* "$SPARK_HOME/"
    rm -rf /tmp/spark-${SPARK_VERSION}-bin-hadoop3
fi

# ---------- 4. 配置 ----------
echo "[4/5] 配置 Spark..."

# 环境变量
if ! grep -q "SPARK_HOME" ~/.bashrc; then
    cat >> ~/.bashrc << 'EOF'
# Apache Spark
export SPARK_HOME=/opt/spark
export PATH=$PATH:$SPARK_HOME/bin:$SPARK_HOME/sbin
export PYSPARK_PYTHON=python3
EOF
fi
source ~/.bashrc 2>/dev/null || true

# spark-env.sh
cp "$SPARK_HOME/conf/spark-env.sh.template" "$SPARK_HOME/conf/spark-env.sh"

cat >> "$SPARK_HOME/conf/spark-env.sh" << SPARKENV

# === PokemonSimulator Spark Config ===
export SPARK_MASTER_HOST=${MASTER_IP}
export SPARK_WORKER_DIR=${SPARK_DATA}/work
export SPARK_LOG_DIR=${SPARK_DATA}/logs
export SPARK_WORKER_MEMORY=2G
export SPARK_WORKER_CORES=2
export SPARK_MASTER_WEBUI_PORT=8080
export SPARK_WORKER_WEBUI_PORT=8081

# Python
export PYSPARK_PYTHON=python3
export PYSPARK_DRIVER_PYTHON=python3
SPARKENV

# spark-defaults.conf
cp "$SPARK_HOME/conf/spark-defaults.conf.template" "$SPARK_HOME/conf/spark-defaults.conf" 2>/dev/null || true

cat >> "$SPARK_HOME/conf/spark-defaults.conf" << SPARKDEF

# === PokemonSimulator ===
spark.master                     spark://${MASTER_IP}:7077
spark.sql.adaptive.enabled       true
spark.sql.adaptive.coalescePartitions.enabled true
spark.serializer                 org.apache.spark.serializer.KryoSerializer
spark.sql.warehouse.dir          hdfs://${MASTER_IP}:8020/user/pokemon/warehouse
SPARKDEF

# workers file
cat > "$SPARK_HOME/conf/workers" << WORKERS
${MASTER_IP}
WORKERS
# TODO: 补充所有 Worker 的 Tailscale IP

# ---------- 5. 启动 ----------
echo "[5/5] 启动 Spark..."

if [ "$ROLE" = "master" ]; then
    "$SPARK_HOME/sbin/start-master.sh"
    echo ""
    echo "=== Spark Master 已启动 ==="
    echo "Web UI:  http://${NODE_IP}:8080"
    echo "RPC:     spark://${NODE_IP}:7077"
    echo ""
    echo "启动 Worker (在每台 Worker 节点执行):"
    echo "  ./install-spark.sh worker <worker-ip> ${MASTER_IP}"

elif [ "$ROLE" = "worker" ]; then
    "$SPARK_HOME/sbin/start-worker.sh" "spark://${MASTER_IP}:7077"
    echo ""
    echo "=== Spark Worker 已启动 ==="
    echo "Web UI: http://${NODE_IP}:8081"
fi

echo ""
echo "=== 安装完成 ==="
echo "提交任务: spark-submit --master spark://${MASTER_IP}:7077 <job.py>"
