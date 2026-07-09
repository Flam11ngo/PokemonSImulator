# PokemonSimulator 分布式实时对战流计算数仓集群部署文档

> **目标**：部署面向高吞吐宝可梦模拟对战日志流的数仓集群（Hadoop + Spark + Kafka），以及 Windows 开发机上的前端/API/数据生成端，打通从数据采集到可视化的完整链路。

---

## 目录

1. 集群拓扑与组网
2. 基础环境与大数据组件自动化部署
3. Hadoop HDFS 分布式存储配置
4. Kafka 分布式消息队列配置
5. Spark 分布式算力与 PySpark 配置
6. 流计算持久化与 Checkpoint
7. 本机开发环境部署
8. 全链路验证

---

## 1. 集群拓扑与组网

### 1.1 节点分配

系统采用多节点协同架构，底层使用 Tailscale 虚拟网络组网打通跨宿主机全互联隧道。

| 主机名 | 角色 | 部署组件 |
|--------|------|---------|
| **`myz`** | 主节点 (Master) | Hadoop NameNode, Spark Master, Kafka Broker, ADS Bridge, Stats API |
| **`lzx`** | 从节点 (Worker) | Hadoop DataNode, Spark Worker, Kafka Broker |
| **`cyc`** | 从节点 (Worker) | Hadoop DataNode, Spark Worker, Kafka Broker |

### 1.2 主机名解析

在集群所有节点的 `/etc/hosts` 中写入 Tailscale 虚拟 IP 映射：

```text
<myz_虚拟网IP>  myz
<lzx_虚拟网IP>  lzx
<cyc_虚拟网IP>  cyc
```

### 1.3 节点互信

在 `myz` 主节点执行：

```bash
ssh-keygen -t rsa
ssh-copy-id hadoop@lzx
ssh-copy-id hadoop@cyc
```

---

## 2. 基础环境与大数据组件自动化部署

### 2.1 标准目录结构

| 路径 | 用途 |
|------|------|
| `/opt/software` | 离线安装包归档（.tar.gz） |
| `/opt/bigdata` | 软件运行主目录 |
| `/opt/bigdata/data` | HDFS 与消息队列落盘载体 |

### 2.2 核心组件版本

| 组件 | 版本 | 安装路径 |
|------|------|---------|
| JDK | 11 (openjdk) | `/usr/lib/jvm/java-11-openjdk-amd64` |
| Hadoop | 3.4.3 | `/opt/bigdata/hadoop` |
| Spark | 3.5.8 | `/opt/bigdata/spark` |
| Zookeeper | 3.8.6 | `/opt/bigdata/zookeeper` |
| Kafka | 3.9.2 | `/opt/bigdata/kafka` |

### 2.3 自动化部署脚本

脚本基于幂等性校验，自动完成目录初始化、组件下载、解压重命名、环境变量注入：

```bash
#!/bin/bash
# 大数据组件自动化部署脚本 (Ubuntu 22.04+)

if [ "$EUID" -eq 0 ]; then
    echo "请勿以 root 运行，使用普通用户（有 sudo 权限）"
    exit 1
fi

SOFTWARE_DIR="/opt/software"
BIGDATA_DIR="/opt/bigdata"
DATA_DIR="/opt/bigdata/data"

echo "=== 第一阶段：初始化集群存储骨架 ==="
sudo mkdir -p "$SOFTWARE_DIR" "$BIGDATA_DIR" "$DATA_DIR"
sudo chown -R $USER:$USER "$SOFTWARE_DIR" "$BIGDATA_DIR"

HADOOP_URL="<hadoop-3.4.3.tar.gz 下载地址>"
SPARK_URL="<spark-3.5.8-bin-hadoop3.tgz 下载地址>"
ZK_URL="<zookeeper-3.8.6-bin.tar.gz 下载地址>"
KAFKA_URL="<kafka_2.13-3.9.2.tgz 下载地址>"

deploy_component() {
    local name=$1 url=$2
    local tar_name=$(basename "$url")
    local target_dir="$BIGDATA_DIR/$name"

    if [ -d "$target_dir" ]; then
        echo "[跳过] $target_dir 已存在"
        return 0
    fi

    if [ ! -f "$SOFTWARE_DIR/$tar_name" ]; then
        echo "[下载] 正在拉取 $name..."
        wget -c "$url" -P "$SOFTWARE_DIR"
    fi

    local tmp_dir=$(mktemp -d -p "$BIGDATA_DIR")
    tar -xf "$SOFTWARE_DIR/$tar_name" -C "$tmp_dir"
    local actual_dir=$(ls "$tmp_dir")
    mv "$tmp_dir/$actual_dir" "$target_dir"
    rm -rf "$tmp_dir"
    echo "[成功] $name → $target_dir"
}

deploy_component "hadoop" "$HADOOP_URL"
deploy_component "spark" "$SPARK_URL"
deploy_component "zookeeper" "$ZK_URL"
deploy_component "kafka" "$KAFKA_URL"

# 环境变量注入
MARKER="# === BIGDATA ECOSYSTEM PROTOCOL ==="
if ! grep -q "$MARKER" ~/.bashrc; then
    cat >> ~/.bashrc << 'EOF'
# === BIGDATA ECOSYSTEM PROTOCOL ===
export JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
export HADOOP_HOME=/opt/bigdata/hadoop
export SPARK_HOME=/opt/bigdata/spark
export ZOOKEEPER_HOME=/opt/bigdata/zookeeper
export KAFKA_HOME=/opt/bigdata/kafka
export PATH=$PATH:$JAVA_HOME/bin:$HADOOP_HOME/bin:$HADOOP_HOME/sbin:$SPARK_HOME/bin:$SPARK_HOME/sbin:$ZOOKEEPER_HOME/bin:$KAFKA_HOME/bin
# ==================================
EOF
    echo "[成功] 环境变量已注入 ~/.bashrc，请执行 source ~/.bashrc"
else
    echo "[跳过] 环境变量已存在"
fi

echo "=================================================="
echo "所有核心大数据软件部署就绪"
echo "=================================================="
```

---

## 3. Hadoop HDFS 分布式存储配置

### 3.1 核心配置

**`core-site.xml`**：
```xml
<property>
    <name>fs.defaultFS</name>
    <value>hdfs://myz:9000</value>
</property>
```

**`hdfs-site.xml`**：
```xml
<property>
    <name>dfs.replication</name>
    <value>2</value>
</property>
```

### 3.2 格式化与启动

```bash
# 在主节点执行
hdfs namenode -format
start-dfs.sh

# 创建数据落盘目录
hdfs dfs -mkdir -p /user/hadoop/realtime_parquet/battle_logs
hdfs dfs -mkdir -p /user/hadoop/realtime_parquet/ui_events
```

---

## 4. Kafka 分布式消息队列配置

### 4.1 Broker 配置

各节点编辑 `config/server.properties`，显式声明 Tailscale 虚拟 IP：

```properties
# 示例: cyc 节点 (broker.id=1)
broker.id=1
listeners=PLAINTEXT://cyc:9092
advertised.listeners=PLAINTEXT://cyc:9092
zookeeper.connect=myz:2181,lzx:2181,cyc:2181
```

### 4.2 创建 Topics

```bash
kafka-topics.sh --create --bootstrap-server myz:9092 \
  --replication-factor 2 --partitions 3 --topic battle.logs

kafka-topics.sh --create --bootstrap-server myz:9092 \
  --replication-factor 2 --partitions 3 --topic player.ui.events
```

### 4.3 Topic 消息格式

详见 `docs/kafka-topics.md`。两类 Topic：

| Topic | 生产者 | 消费者 | 描述 |
|-------|--------|--------|------|
| `battle.logs` | data_daemon.js / 前端打点 | Spark Streaming / kafka_to_sqlite.py | 对战全生命周期 |
| `player.ui.events` | data_daemon.js / 前端打点 | Spark Streaming / kafka_to_sqlite.py | 用户行为全量 |

---

## 5. Spark 分布式算力与 PySpark 配置

### 5.1 物理路径定位

Spark 独立安装于 `/opt/bigdata/spark`，核心 Python 库路径：

- `/opt/bigdata/spark/python`
- `/opt/bigdata/spark/python/lib/py4j-0.10.9.7-src.zip`

### 5.2 虚拟环境绑定

```bash
cd .venv/lib/python3.x/site-packages/
echo "/opt/bigdata/spark/python" > spark_env.pth
echo "/opt/bigdata/spark/python/lib/py4j-0.10.9.7-src.zip" >> spark_env.pth
```

---

## 6. 流计算持久化与 Checkpoint

### 6.1 端到端清洗路径

```
Kafka (ODS) → Spark Structured Streaming → HDFS Parquet (SNAPPY 压缩)
```

- **数据落盘**：`hdfs://myz:9000/user/hadoop/realtime_parquet/battle_logs`
- **Checkpoint**：`hdfs://myz:9000/user/hadoop/realtime_checkpoints/battle_logs`

### 6.2 数据分层

| 层 | 存储位置 | 说明 |
|------|---------|------|
| ODS | Kafka topics | 原始 JSON 消息，3 分区 |
| DWD | HDFS Parquet | Spark Streaming 解析清洗后落盘 |
| ADS | SQLite (`/opt/bigdata/pokemon_stats.db`) | kafka_to_sqlite.py 桥聚合写入，14 张表 |

### 6.3 ADS 表结构（VM 端）

| 表名 | 类型 | 说明 |
|------|------|------|
| `meta_species` | 聚合 | 精灵出场次数/使用率 |
| `meta_moves` | 聚合 | 招式使用次数 |
| `meta_items` | 聚合 | 道具携带次数 |
| `meta_abilities` | 聚合 | 特性使用次数 |
| `battle_events` | 明细 | 对战事件流 |
| `ui_events` | 明细 | UI 事件流 |
| `event_counts` | 聚合 | 各事件类型计数 |
| `summary_stats` | 聚合 | 全局 KPI（battles/events/players） |
| `click_stats` | 聚合 | 按钮点击量 |
| `player_stats` | 聚合 | 玩家活跃度 |
| `page_dwell` | 聚合 | 页面访问量 |
| `recent_events` | 明细 | 最近 500 条（预格式化中文） |
| `battle_turns` | 明细 | 回合数据 |
| `player_favorites` | 聚合 | 热门功能 |

---

## 7. 本机开发环境部署

> **目标**：在 Windows 开发机上部署前端/API/数据生成器，通过 VM 集群的 Kafka 和 Stats API 联动。

### 7.1 本机组件

| 组件 | 端口 | 技术栈 | 说明 |
|------|------|--------|------|
| standalone_server.py | 9000 | Python FastAPI | API + WebSocket 对战网关 |
| data_daemon.js | — | Node.js + kafkajs | 模拟对战数据生成器 |
| Vite Dev Server | 5173 | Node.js + Vue 3 | 前端开发服务器 |

### 7.2 本机环境要求

| 组件 | 最低版本 | 验证命令 |
|------|---------|---------|
| Node.js | 18 LTS | `node -v` |
| Python | 3.8+ | `python --version` |
| Git | 2.30+ | `git --version` |

**连通性检查**：
```bash
# VM SSH
ssh hadoop@myz "hostname"                           # 期望: myz

# VM Kafka 端口（在 VM 上执行）
ssh hadoop@myz "ss -tlnp | grep 9092"               # 期望: LISTEN
```

### 7.3 第1步：数据库文件就位

```bash
# 检查游戏数据
ls -lh data/pokemon.db                              # 期望: 约 83MB

# Smogon 数据库同步
bash scripts/sync_smogon.sh

# 创建必要目录
mkdir -p battle_logs/output logs data
```

### 7.4 第2步：Python 虚拟环境

```bash
cd api-server
python -m venv venv
source venv/Scripts/activate                         # Git Bash
# 或: venv\Scripts\activate                         # Windows CMD

pip install --upgrade pip
pip install fastapi "uvicorn[standard]" httpx websockets python-multipart aiofiles kafka-python

# 验证
python -c "from fastapi import FastAPI; from kafka import KafkaProducer; print('OK')"
```

### 7.5 第3步：Node.js 依赖 + 前端构建

```bash
cd PokemonSimulator
npm install                                          # pokemon-showdown + kafkajs
cd engine-adapter && npm install && cd ..
cd frontend && npm install
npx vite build                                       # 生产构建
ls dist/index.html && echo "Build OK"
```

### 7.6 第4步：配置文件

**`frontend/vite.config.js`** — Vite 代理路由：

```js
export default defineConfig({
  server: {
    port: 5173,
    proxy: {
      '/api/v1/stats':  { target: 'http://myz:8080', changeOrigin: true },
      '/api/v1/smogon': { target: 'http://myz:8080', changeOrigin: true },
      '/api':           { target: 'http://localhost:9000', changeOrigin: true },
      '/ws':            { target: 'http://localhost:9000', ws: true, changeOrigin: true },
    },
  },
})
```

| 请求路径 | 目标 | 用途 |
|---------|------|------|
| `/api/v1/stats/*` | VM :8080 | 实时数据（`pokemon_stats.db`） |
| `/api/v1/smogon/*` | VM :8080 | Smogon 排名（`gen91v1_stats.sqlite`） |
| `/api/*` | localhost :9000 | 对战/队伍/埋点 |
| `/ws` | localhost :9000 | WebSocket 对战 |

**Kafka 连接**（`engine-adapter/data_daemon.js`）：
```js
const KAFKA_BROKER = process.env.KAFKA_BROKER || "myz:9092";
```

### 7.7 第5步：启动服务

在 3 个独立终端依次启动：

```bash
# 终端1：Python API（端口 9000）
python api-server/standalone_server.py

# 终端2：Data Daemon（模拟数据源）
scripts\Windows\start_data_daemon.bat

# 终端3：Vite 前端（端口 5173）
cd frontend && npx vite
```

**预期日志**：

```
# API:
[ws] INFO: Loaded 1050 species, 902 moves, 308 abilities
INFO: Uvicorn running on http://0.0.0.0:9000

# Daemon:
Data daemon: 8 battle + 3 UI workers → myz:9092
[10s] battles:45 ui:7  workers:8b+3u

# Vite:
VITE v6.x.x  ready in xxx ms
➜  http://localhost:5173/
```

### 7.8 服务端口总览

| 端口 | 位置 | 服务 | 验证 |
|------|------|------|------|
| 5173 | 本机 | Vite 前端 | `curl http://localhost:5173` |
| 9000 | 本机 | Python API | `curl http://localhost:9000/api/v1/health` |
| 8080 | VM | Stats API | `curl http://myz:8080/api/v1/stats/deep/summary` |
| 9092 | VM | Kafka | `ssh hadoop@myz "ss -tlnp \| grep 9092"` |
| 9000 | VM | HDFS NameNode RPC | — |

### 7.9 本机架构与 VM 关系

```
本机 (Windows)                          VM 集群 (Ubuntu)
════════════════                          ════════════════
data_daemon.js ──→ Kafka ──────────→ kafka_to_sqlite.py → pokemon_stats.db
                                     stats_server.py (:8080)
standalone_server.py (:9000)
  │ POST /api/v1/analytics ──→ Kafka ──→ VM 桥
  │ GET  /api/v1/stats/* ────→ stats_server.py  透传
  │ GET  /api/v1/smogon/* ───→ stats_server.py  透传

Vite (:5173)
  │ proxy /api/v1/stats  → VM:8080
  │ proxy /api/v1/smogon → VM:8080
  │ proxy /api           → localhost:9000
  │ proxy /ws            → localhost:9000
```

---

## 8. 全链路验证

### 8.1 检查清单

| # | 检查项 | 验证命令 | 期望 |
|---|--------|---------|------|
| 1 | API 运行 | `curl localhost:9000/api/v1/health` | `{"ok":true}` |
| 2 | 前端可访问 | 浏览器 `http://localhost:5173` | 首页正常 |
| 3 | VM Kafka 可达 | `ssh hadoop@myz "ss -tlnp \| grep 9092"` | LISTEN |
| 4 | VM Stats 可达 | `curl http://myz:8080/api/v1/stats/deep/summary` | events > 0 |
| 5 | VM Smogon 可达 | `curl "http://myz:8080/api/v1/smogon/filters"` | `{"ok":true}` |
| 6 | 实时看板 | `http://localhost:5173/realtime` | 显示数据 |
| 7 | 分析面板 | `http://localhost:5173/stats` | 显示排名 |
| 8 | VS Bot 对战 | 点击对战 → 选技能 | 动画正常 |
| 9 | Daemon 产数据 | `tail -1 logs/data_daemon.log` | battles > 0 |
| 10 | 埋点到达 VM | `curl http://myz:8080/api/v1/stats/ui/recent` | 有事件 |

### 8.2 一键验证脚本

```bash
#!/bin/bash
# 全链路验证 — 在本地 Windows 执行

check() { curl -s "$1" | grep -q '"ok"' && echo "✅ $2" || echo "❌ $2"; }

echo "========== 1. 本机服务 =========="
check "http://localhost:9000/api/v1/health" "Python API (:9000)"
curl -s -o /dev/null -w "%{http_code}" http://localhost:5173 | grep -q "200" && echo "✅ Vite (:5173)" || echo "❌ Vite"

echo ""
echo "========== 2. VM 集群 =========="
check "http://myz:8080/api/v1/stats/deep/summary" "VM Stats API"
check "http://myz:8080/api/v1/smogon/filters" "VM Smogon API"

echo ""
echo "========== 3. 数据链路 =========="
tail -1 logs/data_daemon.log 2>/dev/null | grep -q "battles" && echo "✅ Daemon" || echo "⚠️ Daemon (optional)"

echo ""
echo "========== 验证完成 =========="
```

### 8.3 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| `localhost:9000` 拒绝连接 | API 未启动 | `python api-server/standalone_server.py` |
| 实时看板显示"离线" | VM stats 不通或代理配置错误 | 检查 `vite.config.js`；`ssh hadoop@myz` 检查 `stats_server.py` |
| TeamBuilder 使用率为空 | Smogon DB 未同步 | `bash scripts/sync_smogon.sh` |
| Daemon 自动退出 | Git Bash nohup 不稳定 | 用 `.bat` 文件或 cmd 启动 |
| `pip install` 失败 | 网络问题 | `pip install -i https://mirrors.aliyun.com/pypi/simple/ ...` |
| VM 不可达 | SSH 密钥或 Tailscale 断连 | `ssh hadoop@myz "hostname"` 测试 |

---

## 附录 A：关键目录结构

```
PokemonSimulator/
├── api-server/
│   ├── standalone_server.py     # 主 API 服务器 (FastAPI)
│   ├── services/
│   │   ├── analytics_service.py # 埋点收集 + Kafka 发送
│   │   └── kafka_producer.py    # Kafka 异步生产者
│   └── config.py                # 全局配置
├── engine-adapter/
│   ├── data_daemon.js           # 模拟数据生成器 (Kafka Producer)
│   └── showdown_daemon.js       # Showdown 引擎桥接 (Node.js)
├── frontend/
│   ├── src/views/               # 7 个 Vue 页面组件
│   ├── src/api/                 # API 封装 (smogon.js, stats.js, client.js)
│   ├── src/utils/               # 埋点工具 (track.js, analytics.js)
│   └── vite.config.js           # Vite 配置 + 代理规则
├── scripts/
│   ├── kafka_to_sqlite.py       # ADS Bridge (VM 端部署)
│   ├── stats_server.py          # Stats HTTP API (VM 端部署)
│   ├── Linux/deploy.sh          # 云服务器一键部署
│   └── sync_smogon.sh           # Smogon DB 同步
├── data/
│   └── pokemon.db               # 游戏静态数据 (1050 精灵/902 技能)
├── smogon_stats/
│   ├── queries.py               # Smogon SQL 查询层
│   └── gen91v1_stats.sqlite     # Smogon 排名数据库 (335MB)
└── docs/
    ├── system-overview.md       # 系统全景文档
    ├── defense.md               # 答辩文档
    ├── kafka-topics.md          # Kafka 消息格式规范
    └── local-deploy.md          # 本文档
```

## 附录 B：ADS 数据流总览

```
前端打点 (HTTP/WS) ──→ standalone_server.py
                            │
data_daemon.js ─────────────┤
                            ▼
                         Kafka (myz:9092)
                     battle.logs   player.ui.events
                            │
              ┌─────────────┼─────────────┐
              ▼             ▼             ▼
     Spark Streaming    kafka_to_sqlite.py    (可选: HDFS Parquet)
     (DWD Parquet)     (ADS 聚合 → SQLite)
                            │
                       stats_server.py (:8080)
                            │
              ┌─────────────┼─────────────┐
              ▼             ▼             ▼
         实时看板      用户分析     Smogon 面板
```
