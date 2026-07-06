#!/usr/bin/env python3
"""Battle stats consumer: Kafka → aggregate → static DB + HDFS.
Reads from Kafka topic 'battle-events', aggregates usage/winrate stats,
periodically writes to pokemon.db and HDFS."""
import json, os, sqlite3, time, glob
from pathlib import Path
from collections import Counter, defaultdict

DATA_DIR = Path(__file__).resolve().parent.parent / "data"
DB_PATH = DATA_DIR / "pokemon.db"
STREAM_DIR = Path(__file__).resolve().parent / "stream_fallback"

KAFKA_BROKERS = os.getenv("KAFKA_BROKERS", "192.168.88.129:9092")
TOPIC = "battle-events"
GROUP_ID = "stats-consumer"

# ── Kafka consumer ──
consumer = None
try:
    from kafka import KafkaConsumer
    consumer = KafkaConsumer(
        TOPIC, bootstrap_servers=KAFKA_BROKERS.split(","),
        group_id=GROUP_ID, auto_offset_reset="earliest",
        value_deserializer=lambda v: json.loads(v.decode("utf-8")),
        consumer_timeout_ms=10000
    )
    print(f"[consumer] Kafka connected: {KAFKA_BROKERS}")
except Exception as e:
    print(f"[consumer] Kafka unavailable ({e}), using file fallback")

# ── Stats accumulators ──
species_usage = Counter()
species_wr = defaultdict(lambda: {"wins": 0, "losses": 0})
move_usage = Counter()
item_usage = Counter()
ability_usage = Counter()

def process(record):
    t = record.get("type", "")
    if t == "turn":
        for ev in record.get("events", []):
            pass  # per-event aggregation if needed
    elif t == "battle_end":
        for p in record.get("teams", []):
            sid = p.get("speciesId", 0)
            if sid:
                species_usage[sid] += 1
                if p.get("side") == record.get("winner"):
                    species_wr[sid]["wins"] += 1
                else:
                    species_wr[sid]["losses"] += 1

def sync_db():
    if not DB_PATH.exists():
        return
    conn = sqlite3.connect(str(DB_PATH))
    conn.execute("""CREATE TABLE IF NOT EXISTS battle_stats (
        stat_type TEXT, entity_id INTEGER, value REAL, count INTEGER,
        updated_at TEXT, PRIMARY KEY(stat_type, entity_id))""")
    now = time.strftime("%Y-%m-%d %H:%M:%S")
    for sid, cnt in species_usage.items():
        wr = species_wr.get(sid, {})
        w, l = wr.get("wins", 0), wr.get("losses", 0)
        rate = round(w / (w + l), 3) if (w + l) > 0 else 0
        conn.execute("INSERT OR REPLACE INTO battle_stats VALUES ('species_usage',?,?,?,?)",
                     (sid, cnt, now))
        conn.execute("INSERT OR REPLACE INTO battle_stats VALUES ('species_winrate',?,?,?,?)",
                     (sid, rate, now))
    for mid, cnt in move_usage.items():
        conn.execute("INSERT OR REPLACE INTO battle_stats VALUES ('move_usage',?,?,?,?)",
                     (mid, cnt, now))
    conn.commit(); conn.close()
    print(f"[consumer] Synced {len(species_usage)} species, {len(move_usage)} moves at {now}")

def run_kafka():
    """Consume from Kafka, sync every 30s."""
    last_sync = time.time()
    for msg in consumer:
        try:
            process(msg.value)
        except Exception as e:
            print(f"[consumer] process error: {e}")
        if time.time() - last_sync > 30:
            sync_db()
            last_sync = time.time()

def run_files():
    """Read from .jsonl fallback files."""
    processed = set()
    while True:
        for fpath in sorted(STREAM_DIR.glob("*.jsonl")):
            ck = f"{fpath.name}:{fpath.stat().st_mtime}"
            if ck in processed:
                continue
            processed.add(ck)
            with open(fpath, encoding="utf-8") as f:
                for line in f:
                    try:
                        process(json.loads(line.strip()))
                    except Exception:
                        pass
        sync_db()
        time.sleep(30)

if __name__ == "__main__":
    if consumer:
        run_kafka()
    else:
        print("[consumer] File mode: syncing every 30s")
        run_files()
