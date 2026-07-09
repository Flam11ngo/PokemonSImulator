"""
Kafka → HDFS bridge. Runs on the Hadoop node.
Consumes player.ui.events + battle.logs, writes to HDFS as JSONL files.
"""
import json
import os
import subprocess
import time
from datetime import datetime
from pathlib import Path
from kafka import KafkaConsumer

HDFS_BASE = "/user/pokemon"
KAFKA_BROKER = "100.107.105.99:9092"
TOPICS = ["player.ui.events", "battle.logs"]
LOCAL_BUFFER_DIR = Path("/tmp/kafka-hdfs-buffer")
LOCAL_BUFFER_DIR.mkdir(parents=True, exist_ok=True)


def hdfs_write(path: str, content: str):
    """Append a line to an HDFS file using `hdfs dfs -appendToFile`."""
    local_tmp = LOCAL_BUFFER_DIR / f"{datetime.now().strftime('%Y%m%d%H%M%S%f')}.tmp"
    local_tmp.write_text(content, encoding="utf-8")
    try:
        subprocess.run(
            ["hdfs", "dfs", "-appendToFile", str(local_tmp), path],
            check=True, capture_output=True, timeout=30,
        )
    except subprocess.CalledProcessError:
        # appendToFile fails if file doesn't exist yet — create it
        subprocess.run(
            ["hdfs", "dfs", "-put", str(local_tmp), path],
            check=True, capture_output=True, timeout=30,
        )
    local_tmp.unlink(missing_ok=True)


def path_for(topic: str) -> str:
    date_str = datetime.now().strftime("%Y-%m-%d")
    if topic == "battle.logs":
        return f"{HDFS_BASE}/battle_logs/{date_str}.jsonl"
    return f"{HDFS_BASE}/ui_events/{date_str}.jsonl"


def main():
    consumer = KafkaConsumer(
        *TOPICS,
        bootstrap_servers=KAFKA_BROKER,
        group_id="hdfs-bridge",
        value_deserializer=lambda v: json.loads(v.decode("utf-8")),
        auto_offset_reset="earliest",
        enable_auto_commit=True,
    )
    print(f"Kafka → HDFS bridge started, topics: {TOPICS}")

    for msg in consumer:
        try:
            line = json.dumps(msg.value, ensure_ascii=False) + "\n"
            hdfs_write(path_for(msg.topic), line)
            print(f"  {msg.topic} → {path_for(msg.topic)}")
        except Exception as e:
            print(f"  [ERROR] {e}")


if __name__ == "__main__":
    main()
