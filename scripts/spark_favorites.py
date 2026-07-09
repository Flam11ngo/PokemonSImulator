"""
PySpark Batch — compute player page dwell time + click counts from Kafka.
Runs every 30s, reads latest events, writes to SQLite.
Usage: spark-submit --packages org.apache.spark:spark-sql-kafka-0-10_2.12:3.5.0 spark_favorites.py
"""
import json, sqlite3, time
from datetime import datetime

BROKER = "myz:9092"
TOPIC = "player.ui.events"
DB_PATH = "/opt/bigdata/pokemon_stats.db"
BATCH_SIZE = 2000  # messages per batch

def compute_and_write():
    """Read from Kafka via spark-submit's batch mode, compute dwell + clicks, write to SQLite."""
    # We use kafka-python for simplicity — no streaming window issues
    from kafka import KafkaConsumer

    consumer = KafkaConsumer(
        TOPIC,
        bootstrap_servers=BROKER,
        auto_offset_reset="latest",
        enable_auto_commit=True,
        value_deserializer=lambda v: json.loads(v.decode("utf-8")),
        consumer_timeout_ms=10000,
    )

    events = []
    for msg in consumer:
        events.append(msg.value)
        if len(events) >= BATCH_SIZE:
            break

    if not events:
        print(f"[{datetime.now():%H:%M:%S}] no new events")
        return

    # ── Compute per-player page dwell time ──
    # Group events by player, sorted by timestamp
    player_events = {}
    for evt in events:
        pid = evt.get("player_id", "?")
        if pid not in player_events:
            player_events[pid] = []
        player_events[pid].append(evt)

    player_features = {}
    for pid, evts in player_events.items():
        evts.sort(key=lambda e: e.get("timestamp", ""))
        features = {}
        clicks = 0
        dwell = 0.0
        prev_ts = None
        prev_page = None

        for evt in evts:
            event = evt.get("event", "")
            data = evt.get("data", {}) or {}
            ts_str = evt.get("timestamp", "")

            # Dwell time: time between consecutive page_view events
            if event == "page_view":
                page = data.get("page", "/")
                features[page] = features.get(page, 0) + 1
                if prev_ts and prev_page:
                    try:
                        curr = datetime.fromisoformat(ts_str.replace("Z", "+00:00"))
                        prev = datetime.fromisoformat(prev_ts.replace("Z", "+00:00"))
                        diff = (curr - prev).total_seconds()
                        if 0 < diff < 600:
                            dwell += diff
                    except Exception:
                        pass
                prev_ts = ts_str
                prev_page = page

            if event == "ui_click":
                clicks += 1
                el = data.get("element", "?")
                features[el] = features.get(el, 0) + 2  # weighted

            if event == "team_save":
                features["💾 保存队伍"] = features.get("💾 保存队伍", 0) + 5

            if event == "matchmaking_join":
                features["⚔️ 匹配对战"] = features.get("⚔️ 匹配对战", 0) + 4

        player_features[pid] = {
            "features": features,
            "clicks": clicks,
            "dwell": dwell,
        }

    # ── Write to SQLite ──
    conn = sqlite3.connect(DB_PATH, timeout=10)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS player_favorites (
            player_id TEXT, feature TEXT, score REAL,
            clicks INTEGER, dwell_seconds REAL,
            updated TEXT, PRIMARY KEY (player_id, feature)
        )
    """)
    now = datetime.now().isoformat()
    total = 0
    for pid, pf in player_features.items():
        for feat, score in pf["features"].items():
            conn.execute(
                "INSERT OR REPLACE INTO player_favorites (player_id, feature, score, clicks, dwell_seconds, updated) "
                "VALUES (?,?,?,?,?,?)",
                (pid, feat, score, pf["clicks"], pf["dwell"], now)
            )
            total += 1
    conn.commit()
    conn.close()
    print(f"[{datetime.now():%H:%M:%S}] {len(events)} events → {total} features for {len(player_features)} players (dwell={sum(pf['dwell'] for pf in player_features.values()):.0f}s)")

    # ── Also compute aggregate page dwell (across all players) ──
    conn2 = sqlite3.connect(DB_PATH, timeout=10)
    conn2.execute("""
        CREATE TABLE IF NOT EXISTS page_dwell (
            page TEXT PRIMARY KEY,
            total_dwell_seconds REAL DEFAULT 0,
            visit_count INTEGER DEFAULT 0,
            updated TEXT
        )
    """)
    page_stats = {}
    for pid, pf in player_features.items():
        for feat, score in pf["features"].items():
            if feat and feat.startswith("/"):
                if feat not in page_stats:
                    page_stats[feat] = {"dwell": 0, "visits": 0}
                page_stats[feat]["dwell"] += pf["dwell"]
                page_stats[feat]["visits"] += score

    for page, stats in page_stats.items():
        conn2.execute(
            "INSERT OR REPLACE INTO page_dwell (page, total_dwell_seconds, visit_count, updated) VALUES (?,?,?,?)",
            (page, stats["dwell"], stats["visits"], now)
        )
    conn2.commit()
    conn2.close()

    consumer.close()


# ── Main loop ──
print(f"PySpark Favorites (batch) — reading {TOPIC} every 30s")
while True:
    try:
        compute_and_write()
    except Exception as e:
        print(f"[{datetime.now():%H:%M:%S}] error: {e}")
    time.sleep(30)
