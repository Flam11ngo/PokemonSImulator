#!/bin/bash
# ============================================================
# Kafka Topics 初始化脚本 (Person C)
# 在任意一台 Kafka 节点执行
# ============================================================
set -euo pipefail

BOOTSTRAP_SERVER="${1:-100.107.105.99:9092}"

echo "=== 创建 PokemonSimulator Kafka Topics ==="
echo "Bootstrap Server: $BOOTSTRAP_SERVER"

KAFKA_BIN="/opt/kafka/bin"

# 等待 Kafka 就绪
echo "等待 Kafka 就绪..."
for i in $(seq 1 30); do
    if $KAFKA_BIN/kafka-broker-api-versions.sh --bootstrap-server "$BOOTSTRAP_SERVER" &>/dev/null; then
        echo "Kafka 已就绪"
        break
    fi
    sleep 2
done

create_topic() {
    local name="$1"
    local partitions="${2:-3}"
    local replication="${3:-2}"

    $KAFKA_BIN/kafka-topics.sh --bootstrap-server "$BOOTSTRAP_SERVER" \
        --create --if-not-exists \
        --topic "$name" \
        --partitions "$partitions" \
        --replication-factor "$replication" \
        --config "retention.ms=604800000" \
        --config "cleanup.policy=delete"

    echo "  ✓ Topic '$name' (partitions=$partitions, rf=$replication)"
}

# === 核心 Topics ===
create_topic "battle.requests"  3 2    # 对战请求
create_topic "battle.results"   3 2    # 对战结果
create_topic "battle.events"    6 2    # 对战事件（高吞吐）
create_topic "battle.analytics" 3 2    # 分析聚合

echo ""
echo "=== Topics 列表 ==="
$KAFKA_BIN/kafka-topics.sh --bootstrap-server "$BOOTSTRAP_SERVER" --list

echo ""
echo "=== 初始化完成 ==="
