#!/bin/bash
# ============================================================
# Hadoop HDFS 集群安装脚本 (Person B)
# 在 3 台物理机上分别执行，通过 Tailscale IP 互联
#
# 用法:
#   Node1 (NameNode):  ./install-hadoop.sh namenode 100.107.105.99
#   Node2 (DataNode):  ./install-hadoop.sh datanode <node2-tailscale-ip> 100.107.105.99
#   Node3 (DataNode):  ./install-hadoop.sh datanode <node3-tailscale-ip> 100.107.105.99
# ============================================================
set -euo pipefail

HADOOP_VERSION="3.3.6"
HADOOP_HOME="/opt/hadoop"
HADOOP_DATA="/data/hadoop"
REPLICATION_FACTOR=2

ROLE="${1:-}"
NODE_IP="${2:-}"
NAMENODE_IP="${3:-100.107.105.99}"

if [ -z "$ROLE" ] || [ -z "$NODE_IP" ]; then
    echo "用法: $0 <namenode|datanode> <本机Tailscale_IP> [NameNode_IP]"
    exit 1
fi

echo "=== 安装 Hadoop $HADOOP_VERSION ($ROLE) ==="
echo "本机 IP: $NODE_IP"
echo "NameNode IP: $NAMENODE_IP"

# ---------- 1. 安装 Java ----------
echo "[1/6] 检查 Java..."
if ! command -v java &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq openjdk-11-jdk
fi
echo "Java: $(java -version 2>&1 | head -1)"

# ---------- 2. 创建用户和目录 ----------
echo "[2/6] 创建目录..."
sudo mkdir -p "$HADOOP_HOME" "$HADOOP_DATA/name" "$HADOOP_DATA/data"
sudo chown -R "$USER:$USER" "$HADOOP_HOME" "$HADOOP_DATA"

# ---------- 3. 下载 Hadoop ----------
echo "[3/6] 下载 Hadoop $HADOOP_VERSION..."
if [ ! -f "/tmp/hadoop-${HADOOP_VERSION}.tar.gz" ]; then
    wget -q --show-progress \
        "https://archive.apache.org/dist/hadoop/common/hadoop-${HADOOP_VERSION}/hadoop-${HADOOP_VERSION}.tar.gz" \
        -O "/tmp/hadoop-${HADOOP_VERSION}.tar.gz"
fi

if [ ! -d "$HADOOP_HOME/bin" ]; then
    tar -xzf "/tmp/hadoop-${HADOOP_VERSION}.tar.gz" -C /tmp/
    mv /tmp/hadoop-${HADOOP_VERSION}/* "$HADOOP_HOME/"
    rm -rf /tmp/hadoop-${HADOOP_VERSION}
fi

# ---------- 4. 配置环境变量 ----------
echo "[4/6] 配置环境变量..."
if ! grep -q "HADOOP_HOME" ~/.bashrc; then
    cat >> ~/.bashrc << 'EOF'
# Hadoop
export HADOOP_HOME=/opt/hadoop
export HADOOP_CONF_DIR=$HADOOP_HOME/etc/hadoop
export PATH=$PATH:$HADOOP_HOME/bin:$HADOOP_HOME/sbin
export HDFS_NAMENODE_USER=$USER
export HDFS_DATANODE_USER=$USER
export HDFS_SECONDARYNAMENODE_USER=$USER
EOF
fi
source ~/.bashrc 2>/dev/null || true

# ---------- 5. 配置 Hadoop ----------
echo "[5/6] 配置 Hadoop XML..."

# core-site.xml
cat > "$HADOOP_HOME/etc/hadoop/core-site.xml" << CORE
<?xml version="1.0"?>
<configuration>
    <property>
        <name>fs.defaultFS</name>
        <value>hdfs://${NAMENODE_IP}:8020</value>
    </property>
    <property>
        <name>hadoop.tmp.dir</name>
        <value>${HADOOP_DATA}/tmp</value>
    </property>
    <property>
        <name>hadoop.http.staticuser.user</name>
        <value>$USER</value>
    </property>
</configuration>
CORE

# hdfs-site.xml
cat > "$HADOOP_HOME/etc/hadoop/hdfs-site.xml" << HDFS
<?xml version="1.0"?>
<configuration>
    <property>
        <name>dfs.namenode.name.dir</name>
        <value>${HADOOP_DATA}/name</value>
    </property>
    <property>
        <name>dfs.datanode.data.dir</name>
        <value>${HADOOP_DATA}/data</value>
    </property>
    <property>
        <name>dfs.replication</name>
        <value>${REPLICATION_FACTOR}</value>
    </property>
    <property>
        <name>dfs.namenode.rpc-bind-host</name>
        <value>0.0.0.0</value>
    </property>
    <property>
        <name>dfs.namenode.http-address</name>
        <value>0.0.0.0:9870</value>
    </property>
    <property>
        <name>dfs.datanode.http-address</name>
        <value>0.0.0.0:9864</value>
    </property>
    <property>
        <name>dfs.permissions.enabled</name>
        <value>false</value>
    </property>
    <property>
        <name>dfs.webhdfs.enabled</name>
        <value>true</value>
    </property>
</configuration>
HDFS

# workers file (DataNode 列表)
cat > "$HADOOP_HOME/etc/hadoop/workers" << WORKERS
${NAMENODE_IP}
WORKERS
# TODO: 补充所有 DataNode 的 Tailscale IP，每行一个

# hadoop-env.sh - 设置 Java 路径
JAVA_HOME_PATH=$(readlink -f $(which java) | sed 's:/bin/java::')
sed -i "s|^#\?export JAVA_HOME=.*|export JAVA_HOME=${JAVA_HOME_PATH}|" \
    "$HADOOP_HOME/etc/hadoop/hadoop-env.sh"

echo "配置完成。core-site.xml 和 hdfs-site.xml 已生成。"

# ---------- 6. 初始化 ----------
echo "[6/6] 初始化..."
if [ "$ROLE" = "namenode" ]; then
    # 格式化 NameNode (仅首次执行!)
    if [ ! -d "$HADOOP_DATA/name/current" ]; then
        echo "格式化 NameNode..."
        "$HADOOP_HOME/bin/hdfs" namenode -format -force
    else
        echo "NameNode 已格式化，跳过。"
    fi
    # 启动
    echo "启动 HDFS (NameNode + DataNode)..."
    "$HADOOP_HOME/sbin/start-dfs.sh"
    echo ""
    echo "=== Hadoop NameNode 已启动 ==="
    echo "Web UI:  http://${NODE_IP}:9870"
    echo "RPC:     hdfs://${NODE_IP}:8020"
    echo ""
    echo "检查进程: jps"
    echo "创建目录: hdfs dfs -mkdir -p /user/pokemon/static"
elif [ "$ROLE" = "datanode" ]; then
    echo "启动 DataNode..."
    "$HADOOP_HOME/bin/hdfs" --daemon start datanode
    echo ""
    echo "=== Hadoop DataNode 已启动 ==="
    echo "Web UI: http://${NODE_IP}:9864"
fi

echo ""
echo "=== 安装完成 ==="
echo "验证: hdfs dfsadmin -report"
