#!/bin/bash
# ============================================================
#  PokemonSimulator — VM 集群进程一键启动
#  在 Ubuntu 集群节点上运行
#
#  用法（全部通过环境变量配置）:
#    bash vm-setup.sh
#
#  环境变量:
#    KAFKA_HOME  Kafka 安装目录         default: ~/kafka
#    DB_PATH     SQLite 数据库路径      default: /opt/bigdata/pokemon_stats.db
#    STATS_PORT  Stats API 监听端口      default: 8080
#    LOG_DIR     日志目录              default: ~/logs
#
#  注意: 本脚本假设 Kafka、bridge 脚本、stats 脚本已在 VM 上就位
#        桥脚本 (kafka_to_sqlite.py) 和 stats 脚本由 deploy.sh 首次同步
# ============================================================
set -e

KAFKA_HOME="${KAFKA_HOME:-$HOME/kafka}"
DB_PATH="${DB_PATH:-/opt/bigdata/pokemon_stats.db}"
STATS_PORT="${STATS_PORT:-8080}"
BRIDGE_SCRIPT="${BRIDGE_SCRIPT:-$HOME/kafka_to_sqlite.py}"
STATS_SCRIPT="${STATS_SCRIPT:-$HOME/stats_server.py}"
LOG_DIR="${LOG_DIR:-$HOME/logs}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "  ${GREEN}✅${NC} $1"; }
warn() { echo -e "  ${YELLOW}⚠️${NC}  $1"; }
fail() { echo -e "  ${RED}❌${NC} $1"; }
step() { echo -e "\n${YELLOW}═══ [$1]${NC}"; }

mkdir -p "$LOG_DIR" "$(dirname "$DB_PATH")"

# ── Detect Ubuntu packages if missing ──
for pkg in python3 python3-pip sqlite3; do
    if ! command -v "$pkg" &>/dev/null; then
        sudo apt-get install -y -qq "$pkg" 2>/dev/null || fail "Need $pkg"
    fi
done

# ── Python deps ──
pip3 show kafka-python &>/dev/null || pip3 install kafka-python 2>/dev/null || \
    fail "Cannot install kafka-python"

# ── Kafka ──
step "Kafka Broker"
if pgrep -f "kafka.Kafka" > /dev/null 2>&1; then
    ok "Kafka already running"
elif [ -f "$KAFKA_HOME/bin/kafka-server-start.sh" ]; then
    warn "Kafka not running — starting..."
    "$KAFKA_HOME/bin/kafka-server-start.sh" -daemon "$KAFKA_HOME/config/kraft/server.properties"
    sleep 5
    pgrep -f "kafka.Kafka" > /dev/null 2>&1 && ok "Kafka started" || fail "Kafka start failed"
else
    fail "Kafka not found at $KAFKA_HOME"
fi

# ── Bridge ──
step "ADS Bridge ($BRIDGE_SCRIPT → $DB_PATH)"
if pgrep -f kafka_to_sqlite.py > /dev/null 2>&1; then
    ok "Bridge already running"
elif [ -f "$BRIDGE_SCRIPT" ]; then
    warn "Bridge not running — starting..."
    cd "$HOME"
    nohup python3 "$BRIDGE_SCRIPT" > "$LOG_DIR/bridge.log" 2>&1 &
    sleep 2
    pgrep -f kafka_to_sqlite.py > /dev/null 2>&1 && ok "Bridge started" || fail "Bridge start failed"
else
    fail "Bridge script missing: $BRIDGE_SCRIPT"
fi

# ── Stats API ──
step "Stats API (0.0.0.0:$STATS_PORT)"
if pgrep -f stats_server.py > /dev/null 2>&1; then
    ok "Stats API already running"
elif [ -f "$STATS_SCRIPT" ]; then
    warn "Stats API not running — starting..."
    STATS_PORT="$STATS_PORT" nohup python3 "$STATS_SCRIPT" > "$LOG_DIR/stats.log" 2>&1 &
    sleep 2
    pgrep -f stats_server.py > /dev/null 2>&1 && ok "Stats API started" || fail "Stats API start failed"
else
    fail "Stats script missing: $STATS_SCRIPT"
fi

# ── Summary ──
step "Health Check"
echo ""
echo "  Kafka  : $(pgrep -c -f 'kafka.Kafka' 2>/dev/null || echo 0) process(es)"
echo "  Bridge : $(pgrep -c -f kafka_to_sqlite.py 2>/dev/null || echo 0) process(es)"
echo "  Stats  : $(pgrep -c -f stats_server.py 2>/dev/null || echo 0) process(es)"
echo "  DB     : $(ls -lh "$DB_PATH" 2>/dev/null | awk '{print $5}' || echo 'MISSING')"

curl -s "http://localhost:$STATS_PORT/health" 2>/dev/null | grep -q '"ok"' && \
    ok "Stats API responding" || warn "Stats API not responding"
echo ""
echo "============================================"
echo "  VM Setup Complete"
echo "============================================"
