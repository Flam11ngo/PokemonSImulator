"""
SQL-based analytics with persistent connection + TTL cache.
Supports local (SQLite) and cluster (Redis) modes.
"""
import os, sqlite3, time, threading
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent

# DB path from config if local, or None if cluster
try:
    from config import MODE, LOCAL_DB_OUTPUT, REDIS_HOST, REDIS_PORT
except ImportError:
    MODE = os.environ.get("POKEMON_MODE", "local")
    LOCAL_DB_OUTPUT = PROJECT_ROOT / "data" / "output.db"
    REDIS_HOST = "localhost"
    REDIS_PORT = 6379

DB_PATH = os.environ.get("OUTPUT_DB_PATH") or LOCAL_DB_OUTPUT if MODE == "local" else None
DB_PATH = DB_PATH or LOCAL_DB_OUTPUT  # fallback: always use SQLite unless Redis is wired
_tls = threading.local()
_cache = {}
_cache_time = 0.0
_row_count = 0

def _db():
    if not hasattr(_tls, 'conn') or _tls.conn is None:
        _tls.conn = sqlite3.connect(str(DB_PATH))
        _tls.conn.row_factory = sqlite3.Row
        _tls.conn.execute("PRAGMA journal_mode=WAL")
        _tls.conn.execute("PRAGMA synchronous=NORMAL")
        _tls.conn.execute("PRAGMA cache_size=-8000")
    return _tls.conn

def _changed():
    global _row_count
    cur = _db().execute("SELECT COUNT(*) FROM battle_pokemon_states").fetchone()[0]
    if cur != _row_count:
        _row_count = cur
        return True
    return False

def invalidate():
    global _cache_time, _row_count
    _cache_time = 0.0
    _row_count = _db().execute("SELECT COUNT(*) FROM battle_pokemon_states").fetchone()[0]

def _cached(key, fn):
    global _cache_time
    if key in _cache and time.time() - _cache_time < 0.5 and not _changed():
        return _cache[key]
    result = fn()
    _cache[key] = result
    _cache_time = time.time()
    return result

def _has_data():
    return _db().execute("SELECT COUNT(*) FROM battle_pokemon_states").fetchone()[0] > 0


# ── Species / Moves / Items / Abilities ──────────────────────

def get_meta_species():
    return _cached("species", lambda: [dict(r) for r in _db().execute("""
        WITH pa AS (
            SELECT battle_id, side_index, pokemon_index, species_id,
                   MAX(fainted) as ko, AVG(hp_pct) as hp
            FROM battle_pokemon_states GROUP BY 1,2,3,4
        )
        SELECT species_id, COUNT(*) as appearances,
               ROUND(AVG(hp),1) as avg_hp_pct,
               SUM(ko) as times_koed,
               ROUND(CAST(SUM(ko) AS REAL)/MAX(1,COUNT(*))*100,1) as ko_rate
        FROM pa GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()])

def get_meta_moves():
    return _cached("moves", lambda: [dict(r) for r in _db().execute("""
        SELECT CAST(v.value AS INTEGER) as move_id,
               COUNT(DISTINCT bps.battle_id||'-'||bps.species_id) as times_seen
        FROM battle_pokemon_states bps, json_each(bps.move_ids) v
        GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()])

def get_meta_items():
    return _cached("items", lambda: [dict(r) for r in _db().execute("""
        SELECT item_id,
               COUNT(DISTINCT battle_id||'-'||species_id) as uses,
               ROUND(AVG(hp_pct),1) as avg_hp
        FROM battle_pokemon_states WHERE item_id>0
        GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()])

def get_meta_abilities():
    return _cached("abilities", lambda: [dict(r) for r in _db().execute("""
        SELECT ability_id,
               COUNT(DISTINCT battle_id||'-'||species_id) as uses
        FROM battle_pokemon_states WHERE ability_id>0
        GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()])


# ── Species detail ────────────────────────────────────────────

def get_species_moves(sid):
    return [dict(r) for r in _db().execute("""
        WITH pa AS (
            SELECT DISTINCT battle_id, side_index, pokemon_index, move_ids
            FROM battle_pokemon_states WHERE species_id=?
        )
        SELECT CAST(v.value AS INTEGER) as move_id,
               COUNT(*) as usage_count,
               RANK() OVER (ORDER BY COUNT(*) DESC) as rank
        FROM pa, json_each(pa.move_ids) v
        GROUP BY 1 ORDER BY 2 DESC, 1
    """, (sid,)).fetchall()]

def get_species_items(sid):
    return [dict(r) for r in _db().execute("""
        WITH pa AS (
            SELECT DISTINCT battle_id, side_index, pokemon_index, item_id
            FROM battle_pokemon_states WHERE species_id=? AND item_id>0
        )
        SELECT item_id, COUNT(*) as usage_count,
               RANK() OVER (ORDER BY COUNT(*) DESC) as rank
        FROM pa GROUP BY 1 ORDER BY 2 DESC, 1
    """, (sid,)).fetchall()]

def get_species_abilities(sid):
    return [dict(r) for r in _db().execute("""
        WITH pa AS (
            SELECT DISTINCT battle_id, side_index, pokemon_index, ability_id
            FROM battle_pokemon_states WHERE species_id=? AND ability_id>0
        )
        SELECT ability_id, COUNT(*) as usage_count,
               RANK() OVER (ORDER BY COUNT(*) DESC) as rank
        FROM pa GROUP BY 1 ORDER BY 2 DESC, 1
    """, (sid,)).fetchall()]

def get_species_detail(sid):
    return {"species_id": sid,
            "moves": get_species_moves(sid),
            "items": get_species_items(sid),
            "abilities": get_species_abilities(sid)}


# ── Battle data ───────────────────────────────────────────────

def get_battle_hp_curve():
    return _cached("hp_curve", lambda: [dict(r) for r in _db().execute("""
        SELECT battle_id, turn, side_index,
               ROUND(AVG(hp_pct),1) as avg_hp_pct,
               SUM(fainted) as ko_count, COUNT(*) as alive_count
        FROM battle_pokemon_states
        GROUP BY 1,2,3 ORDER BY 1,2,3
    """).fetchall()])

def get_battle_survival():
    return _cached("survival", lambda: [dict(r) for r in _db().execute("""
        WITH pa AS (
            SELECT battle_id, side_index, pokemon_index, species_id,
                   MAX(fainted) as ko
            FROM battle_pokemon_states GROUP BY 1,2,3,4
        )
        SELECT species_id, COUNT(*) as total_appearances,
               SUM(ko) as total_kos,
               ROUND(CAST(SUM(ko) AS REAL)/MAX(1,COUNT(*))*100,1) as ko_rate
        FROM pa GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()])

def get_pokemon_hp():
    return _cached("pokemon_hp", lambda: [dict(r) for r in _db().execute("""
        SELECT battle_id, turn, side_index, pokemon_index,
               species_id, hp_pct, fainted
        FROM battle_pokemon_states ORDER BY 1,2,3,4
    """).fetchall()])

def get_type_distribution():
    def query():
        import sqlite3
        poke = sqlite3.connect(str(PROJECT_ROOT / "data" / "pokemon.db"))
        types = dict(poke.execute("SELECT id, type1 FROM species").fetchall())
        poke.close()
        rows = _db().execute("""
            SELECT species_id, COUNT(DISTINCT battle_id||'-'||species_id) as appearances
            FROM battle_pokemon_states GROUP BY 1 ORDER BY 2 DESC
        """).fetchall()
        result = {}
        for r in rows:
            t = types.get(r["species_id"], "?")
            result[t] = result.get(t, 0) + r["appearances"]
        return [{"type_id": k, "appearances": v} for k, v in sorted(result.items(), key=lambda x: -x[1])]
    return _cached("types", query)


# ── Summary ───────────────────────────────────────────────────

def get_analysis_summary():
    def q():
        db = _db()
        b = db.execute("SELECT COUNT(DISTINCT battle_id) FROM battle_pokemon_states").fetchone()[0]
        t = db.execute("SELECT COUNT(DISTINCT battle_id||'-'||turn) FROM battle_pokemon_states").fetchone()[0]
        s = db.execute("SELECT COUNT(DISTINCT species_id) FROM battle_pokemon_states").fetchone()[0]
        return {"ok": True, "data": {"elapsed_seconds": 0.0, "modules": ["sql"],
                "summaries": {"battle": {"total_battles": b, "total_turns": t},
                              "meta": {"total_species_seen": s}}}}
    return _cached("summary", q)

def get_deep_stats_package():
    return {"ok": True, "data": {
        "summary": get_analysis_summary().get("data", {}),
        "species_usage": get_meta_species()[:20],
        "move_usage": get_meta_moves()[:20],
        "item_usage": get_meta_items()[:20],
        "ability_usage": get_meta_abilities()[:20],
        "type_distribution": get_type_distribution(),
        "hp_curve": get_battle_hp_curve(),
        "pokemon_hp": get_pokemon_hp(),
        "event_distribution": [],
        "survival": get_battle_survival()[:20],
    }}

# ── Team Synergy ──────────────────────────────────────────────────

def get_team_synergy():
    """Top Pokemon pairs that appear together on the same team."""
    return [dict(r) for r in _db().execute("""
        WITH teams AS (
            SELECT DISTINCT battle_id, side_index, species_id
            FROM battle_pokemon_states
        )
        SELECT a.species_id AS s1, b.species_id AS s2, COUNT(*) AS times
        FROM teams a JOIN teams b
          ON a.battle_id = b.battle_id AND a.side_index = b.side_index
        WHERE a.species_id < b.species_id
        GROUP BY 1, 2
        ORDER BY 3 DESC
        LIMIT 30
    """).fetchall()]


# ── Head-to-Head Win Rate ───────────────────────────────────────

def get_head_to_head(s1: int, s2: int):
    """Head-to-head matchup: species s1 vs species s2 on opposite sides."""
    return [dict(r) for r in _db().execute("""
        WITH matches AS (
            SELECT a.battle_id, a.side_index AS s1_side, b.side_index AS s2_side,
                   SUM(CASE WHEN a.fainted=0 THEN 1 ELSE 0 END) AS s1_alive,
                   SUM(CASE WHEN b.fainted=0 THEN 1 ELSE 0 END) AS s2_alive
            FROM battle_pokemon_states a
            JOIN battle_pokemon_states b ON a.battle_id=b.battle_id
            WHERE a.species_id=? AND b.species_id=? AND a.side_index != b.side_index
            GROUP BY a.battle_id
        )
        SELECT COUNT(*) AS total,
               SUM(CASE WHEN s1_alive > s2_alive THEN 1 ELSE 0 END) AS s1_wins,
               SUM(CASE WHEN s2_alive > s1_alive THEN 1 ELSE 0 END) AS s2_wins,
               ROUND(100.0*SUM(CASE WHEN s1_alive > s2_alive THEN 1 ELSE 0 END)/MAX(1,COUNT(*)),1) AS s1_rate,
               ROUND(100.0*SUM(CASE WHEN s2_alive > s1_alive THEN 1 ELSE 0 END)/MAX(1,COUNT(*)),1) AS s2_rate,
               (SELECT ROUND(AVG(hp_pct),1) FROM battle_pokemon_states WHERE species_id=?) AS s1_avg_hp,
               (SELECT ROUND(AVG(hp_pct),1) FROM battle_pokemon_states WHERE species_id=?) AS s2_avg_hp
        FROM matches
    """, (s1, s2, s1, s2)).fetchall()]


def is_available():
    return _has_data()
