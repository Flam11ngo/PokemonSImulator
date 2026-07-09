"""
Delete Kafka records before today (00:00 UTC), reset bridge, rebuild DB.
Usage: ssh hadoop@myz "python3 /tmp/kafka_cleanup.py"
"""
from kafka import KafkaConsumer, KafkaAdminClient
from kafka.structs import TopicPartition
from datetime import datetime, timezone
import os, time

BROKER = "localhost:9092"
TOPICS = ["battle.logs", "player.ui.events"]
DB_PATH = "/opt/bigdata/pokemon_stats.db"
BRIDGE_GROUP = "ads-bridge-v3"

today = datetime.now(timezone.utc).replace(hour=0, minute=0, second=0, microsecond=0)
TARGET_MS = int(today.timestamp() * 1000)

print(f"=== Kafka Daily Cleanup ===")
print(f"Keep data from: {today.isoformat()}Z ({TARGET_MS}ms)")

# ── 1. Stop bridge ──
print("\n[1/5] Stopping bridge...")
os.system("pkill -9 -f kafka_to_sqlite.py 2>/dev/null")
time.sleep(2)

# ── 2. Delete Kafka records before today ──
print("[2/5] Cleaning Kafka records before today...")
consumer = KafkaConsumer(bootstrap_servers=BROKER)
tp_ts = {}
for topic in TOPICS:
    for p in consumer.partitions_for_topic(topic) or []:
        tp_ts[TopicPartition(topic, p)] = TARGET_MS

offsets = consumer.offsets_for_times(tp_ts)
consumer.close()

delete_map = {}
for tp, oat in offsets.items():
    o = oat.offset if hasattr(oat, 'offset') else oat
    if o and o > 0:
        delete_map[tp] = o
        print(f"  {tp.topic} p{tp.partition}: +{o} records deleted")

if delete_map:
    admin = KafkaAdminClient(bootstrap_servers=BROKER, client_id="daily-cleanup")
    admin.delete_records(delete_map)
    admin.close()
    print(f"  {len(delete_map)} partitions truncated")
else:
    print("  No old records to delete")

# ── 3. Reset consumer group ──
print("[3/5] Resetting consumer group to latest...")
c = KafkaConsumer(
    bootstrap_servers=BROKER, group_id=BRIDGE_GROUP,
    auto_offset_reset="latest", enable_auto_commit=True
)
c.subscribe(TOPICS)
c.poll(timeout_ms=1000)
# seek all assigned partitions to end
for tp in c.assignment():
    end = c.end_offsets([tp]).get(tp)
    if end and end > 1:
        c.seek(tp, end - 1)
c.commit()
c.close()
print(f"  Group '{BRIDGE_GROUP}' → latest")

# ── 4. Rebuild DB ──
print("[4/5] Rebuilding database...")
if os.path.exists(DB_PATH):
    os.remove(DB_PATH)
    print(f"  {DB_PATH} deleted")

# ── 5. Restart bridge ──
print("[5/5] Restarting bridge...")
os.system("cd ~ && nohup python3 kafka_to_sqlite.py > logs/bridge.log 2>&1 &")
time.sleep(2)

# Verify
if os.popen("pgrep -f kafka_to_sqlite.py").read().strip():
    print("  Bridge restarted ✓")
else:
    print("  ⚠ Bridge may have failed to start")

print("\n=== Cleanup complete ===")
print(f"Bridge will consume only today's ({today.strftime('%Y-%m-%d')}) data from Kafka.")
