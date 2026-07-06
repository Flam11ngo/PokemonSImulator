"""
Smogon Stats Query Layer.
Pure SQL functions — no HTTP dependency. Used by:
- standalone_server.py REST endpoints (primary)
- smogon_stats/server.py (legacy, can be deprecated)
- Future: Spark SQL backend (when SMOGON_BACKEND=spark)
"""
import sqlite3
import os
import re
from typing import Optional

# Path to game DB for Chinese name lookups — resolve absolutely from this file's location
_GAME_DB = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "pokemon.db"))
_cn_cache = None  # {table: {english_name: chinese_name}}


def _load_cn_cache():
    """Load Chinese name mappings from game DB (cached)."""
    global _cn_cache
    if _cn_cache is not None:
        return _cn_cache
    _cn_cache = {"moves": {}, "items": {}, "abilities": {}, "species": {}}
    if not os.path.exists(_GAME_DB):
        return _cn_cache
    conn = sqlite3.connect(_GAME_DB)
    for table, col in [("moves", "name"), ("items", "name"), ("abilities", "name")]:
        try:
            for row in conn.execute(f"SELECT {col}, chinese_name FROM {table} WHERE chinese_name != ''"):
                key = re.sub(r'[^a-z0-9]', '', row[0].lower())
                if key and row[1]:
                    _cn_cache[table][key] = row[1]
        except Exception:
            pass
    # Also load species name→chinese mapping from name_mapping table
    try:
        for row in conn.execute("SELECT english, chinese FROM name_mapping WHERE chinese != ''"):
            key = row[0].lower().strip()
            if key and row[1]:
                _cn_cache["species"][key] = row[1]
    except Exception:
        pass
    conn.close()
    return _cn_cache


def _add_cn(items, table):
    """Add chinese_name field to a list of dicts from Smogon data."""
    cache = _load_cn_cache()
    mapping = cache.get(table, {})
    for item in items:
        name_col = "type" if table == "teras" else ("mate" if table == "team" else ("opp" if table == "cc" else "name"))
        key = re.sub(r'[^a-z0-9]', '', (item.get(name_col, "")).lower())
        if key and key in mapping:
            item["chinese_name"] = mapping[key]
    return items


def _get_conn(db_path: str) -> sqlite3.Connection:
    """Open a SQLite connection with row factory."""
    if not os.path.exists(db_path):
        raise FileNotFoundError(f"Database not found: {db_path}")
    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row
    return conn


# ============================================================
# Existing queries (extracted from server.py)
# ============================================================

def get_filters(db_path: str) -> dict:
    """Return available sources, time_buckets, and rating tiers."""
    conn = _get_conn(db_path)
    cur = conn.cursor()
    cur.execute("SELECT DISTINCT source FROM mon ORDER BY source")
    sources = [r["source"] for r in cur.fetchall()]
    cur.execute("SELECT DISTINCT time_bucket FROM mon ORDER BY time_bucket DESC")
    time_buckets = [r["time_bucket"] for r in cur.fetchall()]
    cur.execute("SELECT DISTINCT rating FROM mon ORDER BY rating")
    ratings = [r["rating"] for r in cur.fetchall()]
    conn.close()
    return {"sources": sources, "time_buckets": time_buckets, "ratings": ratings}


def get_pokemon_ranking(
    db_path: str,
    source: str,
    time_bucket: str,
    rating: int,
    search: Optional[str] = None,
    limit: int = 50,
    offset: int = 0,
) -> list[dict]:
    """Return ranked Pokemon list with Chinese names."""
    conn = _get_conn(db_path)
    cur = conn.cursor()
    cache = _load_cn_cache()
    species_cn = cache.get("species", {})

    # Build query (no JOIN — Chinese names applied in Python)
    if search:
        query = """
            SELECT m.name, m.usage, m.viability_ceiling
            FROM mon m
            WHERE m.source = ? AND m.time_bucket = ? AND m.rating = ?
              AND (m.name LIKE ?)
            ORDER BY m.usage DESC LIMIT ? OFFSET ?
        """
        params = [source, time_bucket, rating, f"%{search}%", limit, offset]
    else:
        query = """
            SELECT m.name, m.usage, m.viability_ceiling
            FROM mon m
            WHERE m.source = ? AND m.time_bucket = ? AND m.rating = ?
            ORDER BY m.usage DESC LIMIT ? OFFSET ?
        """
        params = [source, time_bucket, rating, limit, offset]
    cur.execute(query, params)
    results = [dict(r) for r in cur.fetchall()]
    conn.close()

    # Add Chinese names
    for r in results:
        r["chinese_name"] = species_cn.get((r.get("name") or "").lower().strip(), "")
    return results


def get_pokemon_detail(
    db_path: str,
    name: str,
    source: str,
    time_bucket: str,
    rating: int,
) -> dict:
    """Return full detail for a single Pokemon: abilities, moves, items, teras,
    teammates, checks/counters, and spreads."""
    conn = _get_conn(db_path)
    cur = conn.cursor()

    # 1. Main info (Chinese name applied in Python)
    cur.execute("""
        SELECT m.name, m.usage, m.viability_ceiling
        FROM mon m
        WHERE m.name = ? AND m.source = ? AND m.time_bucket = ? AND m.rating = ?
    """, (name, source, time_bucket, rating))
    mon_row = cur.fetchone()
    if not mon_row:
        conn.close()
        return None
    info = dict(mon_row)
    species_cn = _load_cn_cache().get("species", {})
    info["chinese_name"] = species_cn.get((info.get("name") or "").lower().strip(), "")

    # Helper
    def fetch_related(table, select="name, usage", order="usage DESC"):
        cur.execute(
            f"SELECT {select} FROM {table} "
            "WHERE mon = ? AND source = ? AND time_bucket = ? AND rating = ? "
            f"ORDER BY {order}",
            (name, source, time_bucket, rating),
        )
        return [dict(r) for r in cur.fetchall()]

    abilities = fetch_related("ability")
    moves = fetch_related("move")
    items = fetch_related("item")
    teras = fetch_related("tera", select="type, usage")
    spreads = fetch_related("spread", select="nature, evs, usage")

    # Teammates (Chinese names applied in Python)
    cur.execute("""
        SELECT t.mate, t.usage
        FROM team t
        WHERE t.mon = ? AND t.source = ? AND t.time_bucket = ? AND t.rating = ?
        ORDER BY t.usage DESC
    """, (name, source, time_bucket, rating))
    teammates = [dict(r) for r in cur.fetchall()]
    for t in teammates:
        t["chinese_name"] = species_cn.get((t.get("mate") or "").lower().strip(), "")

    # Checks & Counters (Chinese names applied in Python)
    cur.execute("""
        SELECT c.opp, c.percentage, c.stddev
        FROM cc c
        WHERE c.mon = ? AND c.source = ? AND c.time_bucket = ? AND c.rating = ?
        ORDER BY c.percentage DESC
    """, (name, source, time_bucket, rating))
    ccs = [dict(r) for r in cur.fetchall()]
    for c in ccs:
        c["chinese_name"] = species_cn.get((c.get("opp") or "").lower().strip(), "")

    conn.close()
    return {
        "info": info,
        "abilities": _add_cn(abilities, "abilities"),
        "moves": _add_cn(moves, "moves"),
        "items": _add_cn(items, "items"),
        "teras": teras,
        "teammates": teammates,
        "ccs": ccs,
        "spreads": spreads,
    }


# ============================================================
# New queries for the enhanced dashboard
# ============================================================

def get_summary_stats(
    db_path: str,
    source: str,
    time_bucket: str,
    rating: int,
) -> dict:
    """Return KPI summary: total species, avg usage, top item/ability by Pokemon count."""
    conn = _get_conn(db_path)
    cur = conn.cursor()

    cur.execute("""
        SELECT COUNT(DISTINCT name) as total,
               ROUND(AVG(usage) * 100, 2) as avg_usage_pct,
               ROUND(MAX(usage) * 100, 2) as max_usage_pct
        FROM mon WHERE source=? AND time_bucket=? AND rating=?
    """, (source, time_bucket, rating))
    base = dict(cur.fetchone())

    # Top item: count distinct Pokemon using it (more meaningful than SUM(usage))
    cur.execute("""
        SELECT name FROM item WHERE source=? AND time_bucket=? AND rating=?
        GROUP BY name ORDER BY COUNT(DISTINCT mon) DESC LIMIT 1
    """, (source, time_bucket, rating))
    top_item_row = cur.fetchone()
    base["top_item"] = top_item_row["name"] if top_item_row else None

    cur.execute("""
        SELECT name FROM ability WHERE source=? AND time_bucket=? AND rating=?
        GROUP BY name ORDER BY COUNT(DISTINCT mon) DESC LIMIT 1
    """, (source, time_bucket, rating))
    top_ab_row = cur.fetchone()
    base["top_ability"] = top_ab_row["name"] if top_ab_row else None

    cur.execute("""
        SELECT name FROM move WHERE source=? AND time_bucket=? AND rating=?
        GROUP BY name ORDER BY COUNT(DISTINCT mon) DESC LIMIT 1
    """, (source, time_bucket, rating))
    top_move_row = cur.fetchone()
    base["top_move"] = top_move_row["name"] if top_move_row else None

    conn.close()
    return base


def get_usage_trend(
    db_path: str,
    name: str,
    source: str = "smogon",
    rating: int = 1760,
) -> list[dict]:
    """Return monthly usage trend for a single Pokemon across all time buckets."""
    conn = _get_conn(db_path)
    cur = conn.cursor()
    cur.execute("""
        SELECT time_bucket, ROUND(usage * 100, 2) as usage_pct,
               ROUND(viability_ceiling, 1) as vc
        FROM mon
        WHERE name=? AND source=? AND rating=?
        ORDER BY time_bucket
    """, (name, source, rating))
    results = [dict(r) for r in cur.fetchall()]
    conn.close()
    return results


def get_type_distribution(
    db_path: str,
    source: str,
    time_bucket: str,
    rating: int,
    limit: int = 20,
) -> list[dict]:
    """Return tera type distribution across the meta (aggregate usage by type)."""
    conn = _get_conn(db_path)
    cur = conn.cursor()
    cur.execute("""
        SELECT type, ROUND(SUM(usage) * 100, 2) as total_pct
        FROM tera
        WHERE source=? AND time_bucket=? AND rating=?
        GROUP BY type
        ORDER BY total_pct DESC
        LIMIT ?
    """, (source, time_bucket, rating, limit))
    results = [dict(r) for r in cur.fetchall()]
    conn.close()
    return results


def get_top_items_abilities(
    db_path: str,
    source: str,
    time_bucket: str,
    rating: int,
    limit: int = 10,
) -> dict:
    """Return global top items and abilities — ranked by how many Pokemon use them."""
    conn = _get_conn(db_path)
    cur = conn.cursor()

    cur.execute("""
        SELECT name, COUNT(DISTINCT mon) as mon_count
        FROM item
        WHERE source=? AND time_bucket=? AND rating=?
        GROUP BY name ORDER BY mon_count DESC LIMIT ?
    """, (source, time_bucket, rating, limit))
    top_items = [dict(r) for r in cur.fetchall()]

    cur.execute("""
        SELECT name, COUNT(DISTINCT mon) as mon_count
        FROM ability
        WHERE source=? AND time_bucket=? AND rating=?
        GROUP BY name ORDER BY mon_count DESC LIMIT ?
    """, (source, time_bucket, rating, limit))
    top_abilities = [dict(r) for r in cur.fetchall()]

    conn.close()
    return {"items": top_items, "abilities": top_abilities}


def get_meta_moves(
    db_path: str,
    source: str,
    time_bucket: str,
    rating: int,
    limit: int = 20,
) -> list[dict]:
    """Return most-used moves across the meta — ranked by how many Pokemon use them."""
    conn = _get_conn(db_path)
    cur = conn.cursor()
    cur.execute("""
        SELECT name, COUNT(DISTINCT mon) as mon_count
        FROM move
        WHERE source=? AND time_bucket=? AND rating=?
        GROUP BY name ORDER BY mon_count DESC LIMIT ?
    """, (source, time_bucket, rating, limit))
    results = [dict(r) for r in cur.fetchall()]
    conn.close()
    return results
