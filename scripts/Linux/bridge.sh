#!/bin/bash
# Kafka → SQLite bridge manager
# Usage: ./bridge.sh {start|stop|restart|status|logs}

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BRIDGE_SCRIPT="$HOME/kafka_to_sqlite.py"
LOG_FILE="$HOME/logs/bridge.log"
PID_FILE="/tmp/kafka_bridge.pid"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

get_pid() {
    pgrep -f "kafka_to_sqlite.py" | head -1
}

start() {
    if get_pid > /dev/null; then
        echo -e "${YELLOW}Bridge already running (PID $(get_pid))${NC}"
        return 0
    fi
    echo -n "Starting Kafka bridge..."
    cd "$HOME"
    nohup python3 "$BRIDGE_SCRIPT" > "$LOG_FILE" 2>&1 &
    local pid=$!
    echo $pid > "$PID_FILE"
    sleep 3
    if kill -0 $pid 2>/dev/null; then
        echo -e " ${GREEN}OK${NC} (PID $pid)"
        tail -2 "$LOG_FILE"
    else
        echo -e " ${RED}FAILED${NC}"
        tail -5 "$LOG_FILE"
        return 1
    fi
}

stop() {
    local pid=$(get_pid)
    if [ -z "$pid" ]; then
        echo -e "${YELLOW}Bridge not running${NC}"
        return 0
    fi
    echo -n "Stopping bridge (PID $pid)..."
    kill $pid
    sleep 2
    if kill -0 $pid 2>/dev/null; then
        echo -e " ${YELLOW}force killing...${NC}"
        kill -9 $pid 2>/dev/null || true
    fi
    rm -f "$PID_FILE"
    echo -e " ${GREEN}stopped${NC}"
}

restart() {
    stop
    sleep 1
    start
}

status() {
    local pid=$(get_pid)
    if [ -z "$pid" ]; then
        echo -e "${RED}Bridge: NOT RUNNING${NC}"
    else
        echo -e "${GREEN}Bridge: RUNNING${NC} (PID $pid)"
        echo "  Log: $LOG_FILE"
        echo "  Uptime: $(ps -o etime= -p $pid 2>/dev/null | xargs)"
    fi
    # Show data counts
    if [ -f /opt/bigdata/pokemon_stats.db ]; then
        local ui=$(sqlite3 /opt/bigdata/pokemon_stats.db "SELECT COUNT(*) FROM ui_events" 2>/dev/null || echo "?")
        local be=$(sqlite3 /opt/bigdata/pokemon_stats.db "SELECT COUNT(*) FROM battle_events" 2>/dev/null || echo "?")
        echo "  UI events: $ui  |  Battle events: $be"
    fi
}

logs() {
    local lines=${1:-30}
    tail -"$lines" "$LOG_FILE"
}

case "${1:-status}" in
    start)   start ;;
    stop)    stop ;;
    restart) restart ;;
    status)  status ;;
    logs)    logs $2 ;;
    *)       echo "Usage: $0 {start|stop|restart|status|logs [N]}"; exit 1 ;;
esac
