#!/bin/bash
# ============================================
#  Analysis & Stats Services — one-click start
#  VM-side: Kafka bridge + Stats API + Spark
# ============================================
set -e

HOME_DIR=$HOME
SPARK_HOME=/opt/bigdata/spark
LOG_DIR=$HOME_DIR/logs
mkdir -p $LOG_DIR

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
ok()  { echo -e "  ${GREEN}✅ $1${NC}"; }
warn(){ echo -e "  ${YELLOW}⚠️  $1${NC}"; }
fail(){ echo -e "  ${RED}❌ $1${NC}"; }

echo -e "${CYAN}============================================"
echo "  PokemonSimulator — Analysis & Stats"
echo -e "============================================${NC}"
echo ""

# ── 0. Check Kafka ──
echo "[0/3] Checking Kafka..."
if ss -tlnp 2>/dev/null | grep -q 9092; then
    ok "Kafka running on :9092"
else
    warn "Kafka not running! Start with: /home/hadoop/kafka/kafka_start.sh"
fi
echo ""

# ── 1. Kafka Bridge ──
echo "[1/3] Kafka → SQLite Bridge"
pkill -f kafka_to_sqlite.py 2>/dev/null && warn "stopped old bridge" || true
sleep 1
cd $HOME_DIR
nohup python3 kafka_to_sqlite.py > $LOG_DIR/bridge.log 2>&1 &
sleep 3
if pgrep -f kafka_to_sqlite.py > /dev/null; then
    ok "Bridge running (PID $(pgrep -f kafka_to_sqlite.py | head -1))"
else
    fail "Bridge failed — check $LOG_DIR/bridge.log"
fi
echo ""

# ── 2. Stats API ──
echo "[2/3] Stats API Server"
pkill -f stats_server.py 2>/dev/null && warn "stopped old API" || true
sleep 1
cd $HOME_DIR
nohup python3 stats_server.py > $LOG_DIR/stats_api.log 2>&1 &
sleep 2
if pgrep -f stats_server.py > /dev/null; then
    ok "Stats API running on :8080 (PID $(pgrep -f stats_server.py | head -1))"
    curl -s http://localhost:8080/health | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'     DB: {d[\"data\"][\"db\"]}')" 2>/dev/null || true
else
    fail "Stats API failed — check $LOG_DIR/stats_api.log"
fi
echo ""

# ── 3. Spark Favorites ──
echo "[3/3] PySpark — Player Favorites"
pkill -f spark_favorites.py 2>/dev/null && warn "stopped old Spark job" || true
sleep 1
cd $HOME_DIR
nohup $SPARK_HOME/bin/spark-submit \
    --master local[*] \
    --packages org.apache.spark:spark-sql-kafka-0-10_2.12:3.5.0 \
    spark_favorites.py \
    > $LOG_DIR/spark_favorites.log 2>&1 &
sleep 8
if pgrep -f spark_favorites.py > /dev/null; then
    ok "Spark Favorites running (PID $(pgrep -f spark_favorites.py | head -1))"
else
    warn "Spark may still be initializing — check $LOG_DIR/spark_favorites.log"
fi
echo ""

# ── Summary ──
echo -e "${CYAN}============================================"
echo "  Services Status"
echo -e "============================================${NC}"
echo ""
echo "  Kafka:       $(ss -tlnp 2>/dev/null | grep -q 9092 && echo -e "${GREEN}✅ :9092${NC}" || echo -e "${RED}❌${NC}")"
echo "  Bridge:      $(pgrep -f kafka_to_sqlite.py >/dev/null && echo -e "${GREEN}✅${NC}" || echo -e "${RED}❌${NC}")"
echo "  Stats API:   $(pgrep -f stats_server.py >/dev/null && echo -e "${GREEN}✅ :8080${NC}" || echo -e "${RED}❌${NC}")"
echo "  Spark Favs:  $(pgrep -f spark_favorites.py >/dev/null && echo -e "${GREEN}✅${NC}" || echo -e "${YELLOW}⏳ initializing${NC}")"
echo ""
echo "  Logs: $LOG_DIR/"
echo "  API:  curl http://localhost:8080/health"
echo -e "${CYAN}============================================${NC}"
