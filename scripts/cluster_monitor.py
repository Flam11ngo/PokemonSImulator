#!/usr/bin/env python3
"""
Cluster health monitor — checks Kafka, HDFS, and data pipeline status.
Run from Windows or Hadoop node.
Usage: python cluster_monitor.py [--watch]
"""
import json, os, subprocess, sys, time
from datetime import datetime

KAFKA_BROKER = "100.107.105.99:9092"
HADOOP_HOST = "100.107.105.99"
TOPICS = ["player.ui.events", "battle.logs"]
WATCH_INTERVAL = 15  # seconds


def ts(): return datetime.now().strftime("%H:%M:%S")


def check_tcp(host, port, timeout=3):
    import socket
    try:
        s = socket.create_connection((host, port), timeout=timeout)
        s.close()
        return True
    except Exception:
        return False


def check_kafka():
    try:
        from kafka import KafkaAdminClient
        admin = KafkaAdminClient(bootstrap_servers=KAFKA_BROKER, request_timeout_ms=5000)
        topics = admin.list_topics()
        admin.close()
        return True, topics
    except Exception as e:
        return False, str(e)


def check_topic_offsets():
    """Get message counts per topic via kafka-run-class (must run on Hadoop)."""
    if not os.path.exists("/opt/bigdata/kafka/bin/kafka-run-class.sh"):
        return None
    try:
        result = subprocess.run(
            ["/opt/bigdata/kafka/bin/kafka-run-class.sh", "kafka.tools.GetOffsetShell",
             "--broker-list", KAFKA_BROKER, "--topic", ",".join(TOPICS), "--time", "-1"],
            capture_output=True, text=True, timeout=10,
        )
        counts = {}
        for line in result.stdout.strip().split("\n"):
            if ":" in line:
                parts = line.split(":")
                topic = parts[0]
                offset = int(parts[-1]) if parts[-1].isdigit() else 0
                counts[topic] = counts.get(topic, 0) + offset
        return counts
    except Exception:
        return None


def check_hdfs():
    try:
        result = subprocess.run(
            ["hdfs", "dfs", "-df", "-h"], capture_output=True, text=True, timeout=10,
        )
        if result.returncode == 0:
            return True, result.stdout.split("\n")[1] if len(result.stdout.split("\n")) > 1 else "OK"
        return False, result.stderr
    except FileNotFoundError:
        return None, "hdfs not in PATH (run on Hadoop node)"
    except Exception as e:
        return False, str(e)


def check_data_daemon():
    """Check if Node.js data_daemon is running on Windows."""
    try:
        result = subprocess.run(
            ["tasklist", "/fi", "IMAGENAME eq node.exe", "/fo", "csv", "/nh"],
            capture_output=True, text=True, timeout=5, shell=True,
        )
        count = result.stdout.count("node.exe")
        return count > 0, f"{count} node.exe processes"
    except Exception:
        return None, "not on Windows"


def check_ssh():
    """Quick SSH connectivity test."""
    try:
        result = subprocess.run(
            ["ssh", "-o", "ConnectTimeout=5", "-o", "BatchMode=yes",
             f"hadoop@{HADOOP_HOST}", "echo ok"],
            capture_output=True, text=True, timeout=10,
        )
        return result.returncode == 0
    except Exception:
        return False


def print_status():
    print(f"\n{'='*60}")
    print(f"  Cluster Monitor  [{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}]")
    print(f"{'='*60}")

    # Network
    kafka_tcp = check_tcp(HADOOP_HOST, 9092)
    print(f"  Kafka :9092  {'🟢 OK' if kafka_tcp else '🔴 DOWN'}")
    hdfs_tcp  = check_tcp(HADOOP_HOST, 8020)
    print(f"  HDFS  :8020  {'🟢 OK' if hdfs_tcp else '🔴 DOWN'}")
    ssh_ok = check_ssh()
    print(f"  SSH          {'🟢 OK' if ssh_ok else '🔴 DOWN'}")

    # Kafka
    kafka_ok, kafka_info = check_kafka()
    if kafka_ok:
        our_topics = [t for t in TOPICS if t in kafka_info]
        print(f"  Kafka topics {'🟢 OK' if our_topics else '🔴 MISSING'}")
        for t in TOPICS:
            print(f"    - {t} {'✓' if t in kafka_info else '✗'}")
    else:
        print(f"  Kafka        🔴 {kafka_info}")

    # Topic message counts (Hadoop only)
    offsets = check_topic_offsets()
    if offsets:
        print(f"  Messages:")
        for t, n in offsets.items():
            print(f"    - {t}: {n:,}")

    # Data daemon (Windows only)
    daemon_ok, daemon_info = check_data_daemon()
    if daemon_ok is not None:
        print(f"  Data daemon  {'🟢' if daemon_ok else '🔴'} {daemon_info}")

    # HDFS (Hadoop only)
    hdfs_ok, hdfs_info = check_hdfs()
    if hdfs_ok is not None:
        print(f"  HDFS         {'🟢' if hdfs_ok else '🔴'} {hdfs_info}")

    print(f"{'='*60}\n")


def main():
    if "--watch" in sys.argv or "-w" in sys.argv:
        print(f"Watching every {WATCH_INTERVAL}s... (Ctrl+C to stop)")
        try:
            while True:
                os.system("cls" if os.name == "nt" else "clear")
                print_status()
                time.sleep(WATCH_INTERVAL)
        except KeyboardInterrupt:
            print("\nStopped.")
    else:
        print_status()


if __name__ == "__main__":
    main()
