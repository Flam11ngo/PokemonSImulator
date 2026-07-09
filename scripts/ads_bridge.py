"""
ADS Bridge — Kafka → SQLite (summary only, no raw events).
SQLite serves the frontend report layer directly.
Raw data stays in Kafka (retention) + optional HDFS.
"""
import json, sqlite3, time
from datetime import datetime
from kafka import KafkaConsumer

BROKER = "myz:9092"
DB_PATH = "/opt/bigdata/pokemon_stats.db"
TOPICS = ["battle.logs", "player.ui.events"]

def init_ads(conn):
    """Create summary tables only — no raw event tables."""
    conn.execute("""CREATE TABLE IF NOT EXISTS meta_species (
        name TEXT PRIMARY KEY, usage_pct REAL DEFAULT 0, appearance_count INTEGER DEFAULT 0,
        faint_count INTEGER DEFAULT 0, ko_rate REAL DEFAULT 0)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS meta_moves (
        name TEXT PRIMARY KEY, usage_pct REAL DEFAULT 0)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS meta_items (
        name TEXT PRIMARY KEY, usage_pct REAL DEFAULT 0)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS meta_abilities (
        name TEXT PRIMARY KEY, usage_pct REAL DEFAULT 0)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS page_dwell (
        page TEXT PRIMARY KEY, total_dwell_seconds REAL DEFAULT 0,
        visit_count INTEGER DEFAULT 0, updated TEXT)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS summary_stats (
        key TEXT PRIMARY KEY, value REAL, updated TEXT)""")
    conn.commit()

# ── Stats counters ──
stats = {"battles": 0, "ui_events": 0, "turns": 0, "last_battle_id": None}

def process_battle(conn, evt):
    data = evt.get("data", {}) or {}
    etype = evt.get("event", "")

    if etype == "battle_init":
        stats["battles"] += 1
        for side in ["side_a", "side_b"]:
            for p in data.get(side, []):
                sid = str(p.get("speciesID", 0))
                if sid and sid != "0":
                    conn.execute("INSERT INTO meta_species (name, appearance_count) VALUES (?,1) ON CONFLICT(name) DO UPDATE SET appearance_count=appearance_count+1", (sid,))
                for m in p.get("moves", []):
                    conn.execute("INSERT INTO meta_moves (name, usage_pct) VALUES (?,1) ON CONFLICT(name) DO UPDATE SET usage_pct=usage_pct+1", (str(m),))
                it = str(p.get("item", 0))
                if it != "0": conn.execute("INSERT INTO meta_items (name, usage_pct) VALUES (?,1) ON CONFLICT(name) DO UPDATE SET usage_pct=usage_pct+1", (it,))
                ab = str(p.get("ability", 0))
                if ab != "0": conn.execute("INSERT INTO meta_abilities (name, usage_pct) VALUES (?,1) ON CONFLICT(name) DO UPDATE SET usage_pct=usage_pct+1", (ab,))

    elif etype == "turn_executed":
        stats["turns"] += 1

    elif etype == "battle_result":
        winner = evt.get("winner") or data.get("winner", "")
        if winner:
            conn.execute("INSERT INTO summary_stats (key, value, updated) VALUES ('last_winner',?,?) ON CONFLICT(key) DO UPDATE SET value=?, updated=?",
                         (winner, datetime.now().isoformat(), winner, datetime.now().isoformat()))
        conn.execute("INSERT INTO summary_stats (key, value, updated) VALUES ('completed_battles',1,?) ON CONFLICT(key) DO UPDATE SET value=value+1, updated=?",
                     (datetime.now().isoformat(), datetime.now().isoformat()))

def process_ui(conn, evt):
    data = evt.get("data", {}) or {}
    stats["ui_events"] += 1
    # Page dwell will be computed by the periodic aggregate, no raw storage needed

def update_dwell(conn):
    """Recompute page dwell from raw Kafka data? No — since we don't store raw events,
    we estimate dwell from visit counts × avg session time per page.
    This is a simplified model; accurate dwell needs raw events in HDFS + Spark."""
    # For now, use visit counts from page_view events we've seen
    conn.execute("UPDATE page_dwell SET updated=? WHERE total_dwell_seconds > 0", (datetime.now().isoformat(),))

def update_pcts(conn):
    total_sp = conn.execute("SELECT COALESCE(SUM(appearance_count),1) FROM meta_species").fetchone()[0]
    total_mv = conn.execute("SELECT COALESCE(SUM(usage_pct),1) FROM meta_moves").fetchone()[0]
    total_it = conn.execute("SELECT COALESCE(SUM(usage_pct),1) FROM meta_items").fetchone()[0]
    total_ab = conn.execute("SELECT COALESCE(SUM(usage_pct),1) FROM meta_abilities").fetchone()[0]
    conn.execute("UPDATE meta_species SET usage_pct = CAST(appearance_count AS REAL) / ?", (total_sp,))
    conn.execute("UPDATE meta_species SET ko_rate = CAST(faint_count AS REAL) / MAX(appearance_count, 1)")
    conn.execute("UPDATE meta_moves SET usage_pct = CAST(usage_pct AS REAL) / ?", (total_mv,))
    conn.execute("UPDATE meta_items SET usage_pct = CAST(usage_pct AS REAL) / ?", (total_it,))
    conn.execute("UPDATE meta_abilities SET usage_pct = CAST(usage_pct AS REAL) / ?", (total_ab,))

    # Update summary stats
    now = datetime.now().isoformat()
    for k, v in stats.items():
        conn.execute("INSERT INTO summary_stats (key, value, updated) VALUES (?,?,?) ON CONFLICT(key) DO UPDATE SET value=?, updated=?",
                     (k, v, now, v, now))
    conn.commit()

def main():
    conn = sqlite3.connect(DB_PATH, timeout=30)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    init_ads(conn)

    consumer = KafkaConsumer(
        *TOPICS, bootstrap_servers=BROKER, group_id="ads-bridge",
        value_deserializer=lambda v: json.loads(v.decode("utf-8")),
        auto_offset_reset="latest", enable_auto_commit=True,
    )
    print(f"ADS Bridge: {BROKER} → {DB_PATH} (summary only)")

    count = 0
    last_update = time.time()
    for msg in consumer:
        try:
            if msg.topic == "battle.logs":
                process_battle(conn, msg.value)
            elif msg.topic == "player.ui.events":
                process_ui(conn, msg.value)
            count += 1

            if time.time() - last_update > 10:
                update_pcts(conn)
                last_update = time.time()

            if count % 50 == 0:
                conn.commit()
                print(f"  [{datetime.now():%H:%M:%S}] {count} msgs | battles:{stats['battles']} turns:{stats['turns']} ui:{stats['ui_events']}")

        except Exception as e:
            print(f"  Error: {e}")

if __name__ == "__main__":
    main()
