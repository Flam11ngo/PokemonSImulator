"""
ADS Bridge — Kafka → SQLite summary tables.
Pre-computes all frontend reports. API reads summaries only (no raw scans).
"""
import json, sqlite3, time
from datetime import datetime
from kafka import KafkaConsumer

BROKER = "myz:9092"
DB_PATH = "/opt/bigdata/pokemon_stats.db"
TOPICS = ["battle.logs", "player.ui.events"]

def init_ads(conn):
    """Summary tables — one per frontend view."""
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
    conn.execute("""CREATE TABLE IF NOT EXISTS click_stats (
        element TEXT PRIMARY KEY, count INTEGER DEFAULT 0)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS player_stats (
        player_id TEXT PRIMARY KEY, events INTEGER DEFAULT 0, last_seen TEXT)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS summary_stats (
        key TEXT PRIMARY KEY, value REAL, updated TEXT)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS event_counts (
        event_type TEXT PRIMARY KEY, count INTEGER DEFAULT 0)""")
    conn.execute("""CREATE TABLE IF NOT EXISTS recent_events (
        id INTEGER PRIMARY KEY AUTOINCREMENT, ts TEXT, player_id TEXT,
        event TEXT, detail TEXT)""")
    # Index for recent_events cleanup
    conn.execute("CREATE INDEX IF NOT EXISTS idx_recent_id ON recent_events(id)")
    conn.commit()

# ── Page label helpers ──
PAGE_LABEL = {"/":"首页","/matchmaking":"匹配","/teams":"组队","/stats":"统计","/data":"数据","/analytics":"分析"}
CLICK_LABEL = {"btn_confirm":"确认","btn_switch":"换人","btn_move_select":"选招","btn_join_match":"PvP匹配",
               "btn_join_bot":"对战Bot","btn_save_team":"保存队伍","nav_matchmaking":"导航-匹配",
               "nav_teams":"导航-组队","nav_stats":"导航-统计","nav_data":"导航-数据"}

def process_battle(conn, evt):
    data = evt.get("data", {}) or {}
    etype = evt.get("event", "")
    # Count event types
    conn.execute("INSERT INTO event_counts (event_type, count) VALUES (?,1) ON CONFLICT(event_type) DO UPDATE SET count=count+1", (etype,))

    if etype == "battle_init":
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

    elif etype == "battle_result":
        winner = evt.get("winner") or data.get("winner", "")
        result = data.get("result", "draw")

def process_ui(conn, evt):
    data = evt.get("data", {}) or {}
    pid = evt.get("player_id", "?")
    etype = evt.get("event", "")
    ts = evt.get("timestamp", "")
    # Count event types
    conn.execute("INSERT INTO event_counts (event_type, count) VALUES (?,1) ON CONFLICT(event_type) DO UPDATE SET count=count+1", (etype,))

    # Player stats
    conn.execute("INSERT INTO player_stats (player_id, events, last_seen) VALUES (?,1,?) ON CONFLICT(player_id) DO UPDATE SET events=events+1, last_seen=?", (pid, ts, ts))

    if etype == "page_view":
        page = data.get("page", "/")
        # Dwell: simple visit counting, real dwell from timestamp diffs computed in periodic update
        conn.execute("INSERT INTO page_dwell (page, total_dwell_seconds, visit_count, updated) VALUES (?,0,1,?) ON CONFLICT(page) DO UPDATE SET visit_count=visit_count+1, updated=?", (page, ts, ts))

        # Recent event feed
        label = PAGE_LABEL.get(page, page)
        detail = f"浏览了 {label}"
        conn.execute("INSERT INTO recent_events (ts, player_id, event, detail) VALUES (?,?,?,?)", (ts, pid, etype, detail))

    elif etype == "ui_click":
        el = data.get("element", "?")
        conn.execute("INSERT INTO click_stats (element, count) VALUES (?,1) ON CONFLICT(element) DO UPDATE SET count=count+1", (el,))
        label = CLICK_LABEL.get(el, el)
        conn.execute("INSERT INTO recent_events (ts, player_id, event, detail) VALUES (?,?,?,?)", (ts, pid, etype, f"点击了 {label}"))

    elif etype == "player_state":
        f, t = data.get("from","?"), data.get("to","?")
        conn.execute("INSERT INTO recent_events (ts, player_id, event, detail) VALUES (?,?,?,?)", (ts, pid, etype, f"{f} → {t}"))

    elif etype == "team_save":
        tn = data.get("team_name", "?")
        conn.execute("INSERT INTO recent_events (ts, player_id, event, detail) VALUES (?,?,?,?)", (ts, pid, etype, f"保存了队伍「{tn}」"))

    elif etype == "session_start":
        conn.execute("INSERT INTO recent_events (ts, player_id, event, detail) VALUES (?,?,?,?)", (ts, pid, etype, "进入了应用"))

    elif etype == "matchmaking_join":
        conn.execute("INSERT INTO recent_events (ts, player_id, event, detail) VALUES (?,?,?,?)", (ts, pid, etype, "加入了匹配队列"))

def compute_dwell(conn):
    """Recompute page dwell from raw timestamps in recent_events."""
    rows = conn.execute(
        "SELECT player_id, detail, ts FROM recent_events WHERE event='page_view' ORDER BY player_id, ts"
    ).fetchall()
    dwells = {}
    prev = {}
    for pid, detail, ts in rows:
        page = None
        for k, v in PAGE_LABEL.items():
            if v in detail:
                page = k; break
        if not page: continue
        if pid in prev:
            try:
                t1 = datetime.fromisoformat(prev[pid][1].replace("Z", "+00:00"))
                t2 = datetime.fromisoformat(ts.replace("Z", "+00:00"))
                d = (t2 - t1).total_seconds()
                if 0 < d < 600:
                    dwells[prev[pid][0]] = dwells.get(prev[pid][0], 0) + d
            except: pass
        prev[pid] = (page, ts)
    now = datetime.now().isoformat()
    for page, d in dwells.items():
        conn.execute("UPDATE page_dwell SET total_dwell_seconds=total_dwell_seconds+?, updated=? WHERE page=?",
                     (round(d, 1), now, page))

def update_pcts(conn):
    """Recalculate all percentages and summary stats."""
    total_sp = conn.execute("SELECT COALESCE(SUM(appearance_count),1) FROM meta_species").fetchone()[0]
    total_mv = conn.execute("SELECT COALESCE(SUM(usage_pct),1) FROM meta_moves").fetchone()[0]
    total_it = conn.execute("SELECT COALESCE(SUM(usage_pct),1) FROM meta_items").fetchone()[0]
    total_ab = conn.execute("SELECT COALESCE(SUM(usage_pct),1) FROM meta_abilities").fetchone()[0]
    conn.execute("UPDATE meta_species SET usage_pct = CAST(appearance_count AS REAL) / ?", (total_sp,))
    conn.execute("UPDATE meta_species SET ko_rate = CAST(faint_count AS REAL) / MAX(appearance_count, 1)")
    conn.execute("UPDATE meta_moves SET usage_pct = CAST(usage_pct AS REAL) / ?", (total_mv,))
    conn.execute("UPDATE meta_items SET usage_pct = CAST(usage_pct AS REAL) / ?", (total_it,))
    conn.execute("UPDATE meta_abilities SET usage_pct = CAST(usage_pct AS REAL) / ?", (total_ab,))

    # Summary stats
    now = datetime.now().isoformat()
    battles = conn.execute("SELECT COUNT(*) FROM event_counts WHERE event_type='battle_init'").fetchone()[0]
    events = conn.execute("SELECT COALESCE(SUM(count),0) FROM event_counts").fetchone()[0]
    players = conn.execute("SELECT COUNT(*) FROM player_stats").fetchone()[0]
    for k, v in [("battles", battles), ("total_events", events), ("players", players)]:
        conn.execute("INSERT INTO summary_stats (key, value, updated) VALUES (?,?,?) ON CONFLICT(key) DO UPDATE SET value=?, updated=?", (k, v, now, v, now))

    # Clean old recent events (keep last 500)
    conn.execute("DELETE FROM recent_events WHERE id NOT IN (SELECT id FROM recent_events ORDER BY id DESC LIMIT 500)")
    conn.commit()

def main():
    conn = sqlite3.connect(DB_PATH, timeout=30)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    init_ads(conn)

    consumer = KafkaConsumer(
        *TOPICS, bootstrap_servers=BROKER, group_id="ads-bridge-v2",
        value_deserializer=lambda v: json.loads(v.decode("utf-8")),
        auto_offset_reset="latest", enable_auto_commit=True,
    )
    print(f"ADS Bridge: {BROKER} → {DB_PATH} (summary-only mode)")

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
                compute_dwell(conn)
                update_pcts(conn)
                last_update = time.time()

            if count % 30 == 0:
                conn.commit()

        except Exception as e:
            print(f"  Error: {e}")

if __name__ == "__main__":
    main()
