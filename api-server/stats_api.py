"""
PokemonSimulator 分析数据服务

优先使用 SQLite 直接查询（毫秒级），CSV 文件作为回退。
前端 API 格式不变。
"""

import csv
import json
import os
import glob
from pathlib import Path
from typing import Optional, Callable

PROJECT_ROOT = Path(__file__).resolve().parent.parent
ANALYTICS_DIR = PROJECT_ROOT / "logs" / "analytics"
DATA_DIR = PROJECT_ROOT / "data"

# Lazy-import sql_analytics to avoid circular dependency
_sql_analytics = None


def _get_sql():
    global _sql_analytics
    if _sql_analytics is None:
        from sql_analytics import is_available
        if is_available():
            import sql_analytics as sa
            _sql_analytics = sa
    return _sql_analytics


def _sql_first(sql_fn: Callable, csv_fn: Callable, *args):
    """Try SQL first; fall back to CSV if no data or error."""
    sa = _get_sql()
    if sa is not None:
        try:
            result = sql_fn(sa, *args)
            if result:
                return result
        except Exception:
            pass  # fall through to CSV
    return csv_fn(*args)


def _read_csv(csv_dir: str) -> list[dict]:
    """Read CSV from Spark output directory (contains part-*.csv files)."""
    dir_path = os.path.join(ANALYTICS_DIR, csv_dir)
    if not os.path.isdir(dir_path):
        # Spark writes as <name>.csv/ directory
        alt_path = os.path.join(ANALYTICS_DIR, csv_dir + ".csv")
        if os.path.isdir(alt_path):
            dir_path = alt_path
        else:
            return []

    part_files = glob.glob(os.path.join(dir_path, "part-*.csv"))
    if not part_files:
        # Try direct file
        if os.path.isfile(dir_path):
            part_files = [dir_path]
        else:
            return []

    rows = []
    for pf in part_files:
        with open(pf, "r", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                rows.append(row)
    return rows


def _read_json(filename: str) -> Optional[dict]:
    """Read a JSON file from ANALYTICS_DIR."""
    path = ANALYTICS_DIR / filename
    if path.exists():
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    return None


# ── Lazy-loaded lookup tables (cached after first load) ──────────
_lookup_cache: dict[str, dict] = {}


def _load_lookup(name: str) -> dict:
    """Load a lookup table from data JSON files. Caches in memory."""
    if name in _lookup_cache:
        return _lookup_cache[name]

    file_map = {
        "species": "species.json",
        "moves": "moves.json",
        "abilities": "abilities.json",
        "items": "items.json",
    }
    filename = file_map.get(name)
    if not filename:
        return {}

    path = DATA_DIR / filename
    if not path.exists():
        _lookup_cache[name] = {}
        return {}

    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Unwrap (data files use {"species": [...], ...} format)
    if isinstance(data, dict):
        items = data.get(name, data.get(next(iter(data.keys()), None), []))
    elif isinstance(data, list):
        items = data
    else:
        items = []

    # Build id -> name dict
    lookup = {}
    for item in items:
        if isinstance(item, dict):
            item_id = item.get("id")
            # Prefer zh_name, then name, then apiName
            name = item.get("zh_name") or item.get("name") or item.get("apiName") or ""
            if item_id is not None:
                lookup[int(item_id)] = name

    _lookup_cache[name] = lookup
    return lookup


def _species_name(species_id) -> str:
    if isinstance(species_id, str):
        try:
            species_id = int(float(species_id))
        except ValueError:
            return species_id
    return _load_lookup("species").get(species_id, f"#{species_id}")


def _species_sprite_url(species_id) -> str:
    """Get local sprite path for a species ID."""
    if isinstance(species_id, str):
        try: species_id = int(float(species_id))
        except ValueError: pass
    return f"/sprites/{species_id}.gif"


def _enrich_species_rows(rows: list, name_key: str = "species_id") -> list:
    """Add sprite_url and species_name to rows that have species_id."""
    for r in rows:
        sid = r.get(name_key, 0)
        if isinstance(sid, str):
            try: sid = int(float(sid))
            except ValueError: pass
        r["species_name"] = _species_name(sid)
        r["sprite_url"] = _species_sprite_url(sid)
    return rows


def _move_name(move_id) -> str:
    if isinstance(move_id, str):
        try:
            move_id = int(float(move_id))
        except ValueError:
            return move_id
    return _load_lookup("moves").get(move_id, f"#{move_id}")


def _ability_name(ability_id) -> str:
    if isinstance(ability_id, str):
        try:
            ability_id = int(float(ability_id))
        except ValueError:
            return ability_id
    return _load_lookup("abilities").get(ability_id, f"#{ability_id}")


def _item_name(item_id) -> str:
    if isinstance(item_id, str):
        try:
            item_id = int(float(item_id))
        except ValueError:
            return item_id
    return _load_lookup("items").get(item_id, f"#{item_id}")


# ── Public API ──────────────────────────────────────────────────

def get_analysis_summary() -> dict:
    """Get the analysis run summary."""
    sa = _get_sql()
    if sa is not None:
        try:
            return sa.get_analysis_summary()
        except Exception:
            pass
    data = _read_json("analysis_summary.json")
    if not data:
        return {"ok": True, "data": None, "message": "No analysis data yet"}
    return {"ok": True, "data": data}


def get_meta_species() -> list[dict]:
    """Pokemon usage ranking (top 100)."""
    def from_sql(sa):
        rows = sa.get_meta_species()
        return [_enrich_species_row(dict(r)) for r in rows]

    def from_csv():
        rows = _read_csv("meta_species_usage.csv")
        result = []
        for r in rows:
            sid = int(float(r.get("species_id", 0)))
            result.append({
                "species_id": sid,
                "species_name": _species_name(sid),
                "sprite_url": _species_sprite_url(sid),
                "appearances": int(float(r.get("appearances", 0))),
                "avg_hp_pct": float(r.get("avg_hp_pct", 0)),
                "times_koed": int(float(r.get("times_koed", 0))),
                "ko_rate": float(r.get("ko_rate", 0)),
            })
        return result

    return _sql_first(from_sql, from_csv)


def _enrich_species_row(r: dict) -> dict:
    sid = r.get("species_id", 0)
    r["species_name"] = _species_name(sid)
    r["sprite_url"] = _species_sprite_url(sid)
    return r


def _int(v):
    return int(float(v)) if v is not None else 0


def _float(v):
    return float(v) if v is not None else 0.0


def get_meta_moves() -> list[dict]:
    """Move usage ranking (top 100)."""
    def from_sql(sa):
        return [{"move_id": _int(r.get("move_id")), "move_name": _move_name(_int(r.get("move_id"))),
                 "times_seen": _int(r.get("times_seen"))} for r in sa.get_meta_moves()]

    def from_csv():
        rows = _read_csv("meta_move_usage.csv")
        return [{"move_id": _int(r.get("move_id")), "move_name": _move_name(_int(r.get("move_id"))),
                 "times_seen": _int(r.get("times_seen"))} for r in rows]

    return _sql_first(from_sql, from_csv)


def get_meta_items() -> list[dict]:
    """Item usage ranking."""
    def from_sql(sa):
        return [{"item_id": _int(r.get("item_id")), "item_name": _item_name(_int(r.get("item_id"))),
                 "uses": _int(r.get("uses")), "avg_hp": _float(r.get("avg_hp"))}
                for r in sa.get_meta_items() if _int(r.get("item_id")) > 0]

    def from_csv():
        rows = _read_csv("meta_item_usage.csv")
        return [{"item_id": _int(r.get("item_id")), "item_name": _item_name(_int(r.get("item_id"))),
                 "uses": _int(r.get("uses")), "avg_hp": _float(r.get("avg_hp"))}
                for r in rows if _int(r.get("item_id")) > 0]

    return _sql_first(from_sql, from_csv)


def get_meta_abilities() -> list[dict]:
    """Ability usage ranking."""
    def from_sql(sa):
        return [{"ability_id": _int(r.get("ability_id")), "ability_name": _ability_name(_int(r.get("ability_id"))),
                 "uses": _int(r.get("uses"))}
                for r in sa.get_meta_abilities() if _int(r.get("ability_id")) > 0]

    def from_csv():
        rows = _read_csv("meta_ability_usage.csv")
        return [{"ability_id": _int(r.get("ability_id")), "ability_name": _ability_name(_int(r.get("ability_id"))),
                 "uses": _int(r.get("uses"))} for r in rows if _int(r.get("ability_id")) > 0]

    return _sql_first(from_sql, from_csv)


def get_type_distribution() -> list[dict]:
    """Type distribution across all pokemon appearances."""
    def from_sql(sa):
        return [{"type_id": str(r.get("type_id", "?")),
                 "type_name": str(r.get("type_id", "?")),
                 "appearances": _int(r.get("appearances"))}
                for r in sa.get_type_distribution()]

    def from_csv():
        rows = _read_csv("type_distribution.csv")
        return [{"type_name": r.get("type_name", r.get("type_id", "?")),
                 "type_id": r.get("type_id", "?"),
                 "appearances": _int(r.get("appearances"))} for r in rows]

    return _sql_first(from_sql, from_csv)


def get_battle_hp_curve() -> list[dict]:
    """Per-turn HP curves for each side."""
    rows = _read_csv("battle_hp_curve.csv")
    return [{
        "battle_id": r.get("battle_id", ""),
        "turn": int(float(r.get("turn", 0))),
        "side_index": int(float(r.get("side_index", 0))),
        "avg_hp_pct": float(r.get("avg_hp_pct", 0)),
        "ko_count": int(float(r.get("ko_count", 0))),
        "alive_count": int(float(r.get("alive_count", 0))),
    } for r in rows]


def get_event_distribution() -> list[dict]:
    """Event type distribution."""
    rows = _read_csv("event_type_dist.csv")
    return [{
        "event_type": r.get("event_type", ""),
        "count": int(float(r.get("count", 0))),
        "pct": float(r.get("pct", 0)),
    } for r in rows]


def get_battle_survival() -> list[dict]:
    """Pokemon survival / KO rate ranking."""
    def from_sql(sa):
        return [_enrich_species_row({
            "species_id": _int(r.get("species_id")),
            "total_appearances": _int(r.get("total_appearances")),
            "total_kos": _int(r.get("total_kos")),
            "ko_rate": _float(r.get("ko_rate")),
        }) for r in sa.get_battle_survival()]

    def from_csv():
        rows = _read_csv("battle_survival.csv")
        return [_enrich_species_row({
            "species_id": _int(r.get("species_id")),
            "total_appearances": _int(r.get("total_appearances")),
            "total_kos": _int(r.get("total_kos")),
            "ko_rate": _float(r.get("ko_rate")),
        }) for r in rows]

    return _sql_first(from_sql, from_csv)


def get_pokemon_hp() -> list[dict]:
    """Per-pokemon HP trace (individual Pokemon HP per turn)."""
    def from_sql(sa):
        result = []
        for r in sa.get_pokemon_hp():
            sid = _int(r.get("species_id"))
            result.append({
                "battle_id": str(r.get("battle_id", "")),
                "turn": _int(r.get("turn")),
                "side_index": _int(r.get("side_index")),
                "pokemon_index": _int(r.get("pokemon_index")),
                "species_id": sid,
                "species_name": _species_name(sid),
                "sprite_url": _species_sprite_url(sid),
                "hp_pct": _float(r.get("hp_pct")),
                "fainted": bool(r.get("fainted")),
            })
        return result

    def from_csv():
        rows = _read_csv("battle_pokemon_hp")
        return [{
            "battle_id": r.get("battle_id", ""),
            "turn": _int(r.get("turn")),
            "side_index": _int(r.get("side_index")),
            "pokemon_index": _int(r.get("pokemon_index")),
            "species_id": _int(r.get("species_id")),
            "species_name": _species_name(_int(r.get("species_id"))),
            "sprite_url": _species_sprite_url(_int(r.get("species_id"))),
            "hp_pct": _float(r.get("hp_pct")),
            "fainted": r.get("fainted", "false") in ("true", "True", "1"),
        } for r in rows]

    return _sql_first(from_sql, from_csv)


def _filter_csv_by_species(csv_dir: str, species_id: int) -> list[dict]:
    """Read a per-species CSV and filter rows for the given species_id.

    Expected CSV schema: species_id, <entity>_id, usage_count, rank
    Returns only rows matching the given species_id.
    """
    rows = _read_csv(csv_dir)
    result = []
    for r in rows:
        try:
            sid = int(float(r.get("species_id", -1)))
        except (ValueError, TypeError):
            continue
        if sid == species_id:
            result.append(r)
    return result


def get_species_moves(species_id: int) -> list[dict]:
    """Return ranked moves for a specific species."""
    rows = _filter_csv_by_species("meta_species_moves", species_id)
    result = []
    for r in rows:
        mid = int(float(r.get("move_id", 0)))
        result.append({
            "move_id": mid,
            "move_name": _move_name(mid),
            "usage_count": int(float(r.get("usage_count", 0))),
            "rank": int(float(r.get("rank", 0))),
        })
    result.sort(key=lambda x: x["rank"])
    return result


def get_species_items(species_id: int) -> list[dict]:
    """Return ranked items for a specific species."""
    rows = _filter_csv_by_species("meta_species_items", species_id)
    result = []
    for r in rows:
        iid = int(float(r.get("item_id", 0)))
        result.append({
            "item_id": iid,
            "item_name": _item_name(iid),
            "usage_count": int(float(r.get("usage_count", 0))),
            "rank": int(float(r.get("rank", 0))),
        })
    result.sort(key=lambda x: x["rank"])
    return result


def get_species_abilities(species_id: int) -> list[dict]:
    """Return ranked abilities for a specific species."""
    rows = _filter_csv_by_species("meta_species_abilities", species_id)
    result = []
    for r in rows:
        aid = int(float(r.get("ability_id", 0)))
        result.append({
            "ability_id": aid,
            "ability_name": _ability_name(aid),
            "usage_count": int(float(r.get("usage_count", 0))),
            "rank": int(float(r.get("rank", 0))),
        })
    result.sort(key=lambda x: x["rank"])
    return result


def get_species_detail(species_id: int) -> dict:
    """Return all three rankings for a single species."""
    sa = _get_sql()
    if sa is not None:
        try:
            detail = sa.get_species_detail(species_id)
            # Enrich with names
            for m in detail.get("moves", []):
                m["move_name"] = _move_name(_int(m.get("move_id")))
            for i in detail.get("items", []):
                i["item_name"] = _item_name(_int(i.get("item_id")))
            for a in detail.get("abilities", []):
                a["ability_name"] = _ability_name(_int(a.get("ability_id")))
            return detail
        except Exception:
            pass

    # CSV fallback
    return {
        "species_id": species_id,
        "moves": get_species_moves(species_id),
        "items": get_species_items(species_id),
        "abilities": get_species_abilities(species_id),
    }


# ── Team Synergy ──────────────────────────────────────────────────

def get_team_synergy():
    sa = _get_sql()
    if sa is not None:
        try:
            pairs = sa.get_team_synergy()
            for p in pairs:
                p["s1_name"] = _species_name(_int(p.get("s1")))
                p["s2_name"] = _species_name(_int(p.get("s2")))
            return pairs
        except Exception: pass
    return []

def get_head_to_head(s1: int = 0, s2: int = 0):
    sa = _get_sql()
    if sa is not None and s1 > 0 and s2 > 0:
        try:
            rows = sa.get_head_to_head(s1, s2)
            if rows and rows[0].get("total", 0) > 0:
                r = rows[0]
                r["s1_name"] = _species_name(s1)
                r["s2_name"] = _species_name(s2)
                r["s1_sprite"] = _species_sprite_url(s1)
                r["s2_sprite"] = _species_sprite_url(s2)
                r["s1_id"] = s1
                r["s2_id"] = s2
                # Type info from pokemon.db
                try:
                    import sqlite3
                    poke = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
                    VALID_TYPES = {"Normal","Fire","Water","Electric","Grass","Ice","Fighting",
                        "Poison","Ground","Flying","Psychic","Bug","Rock","Ghost","Dragon","Dark","Steel","Fairy"}
                    for sid, key in [(s1, "s1_types"), (s2, "s2_types")]:
                        t = poke.execute("SELECT type1, type2 FROM species WHERE id=?", (sid,)).fetchone()
                        if t:
                            t2 = f"/{t[1]}" if t[1] and t[1] in VALID_TYPES else ""
                            r[key] = f"{t[0]}{t2}"
                    poke.close()
                except: pass
                return r
        except Exception: pass
    return {"total": 0}


def get_deep_stats_package() -> dict:
    """Get a full package of all deep stats (for WebSocket or single request)."""
    sa = _get_sql()
    if sa is not None:
        try:
            return sa.get_deep_stats_package()
        except Exception:
            pass
    return {
        "ok": True,
        "data": {
            "summary": (_read_json("analysis_summary.json") or {}),
            "species_usage": get_meta_species()[:20],
            "move_usage": get_meta_moves()[:20],
            "item_usage": get_meta_items()[:20],
            "ability_usage": get_meta_abilities()[:20],
            "type_distribution": get_type_distribution(),
            "hp_curve": get_battle_hp_curve(),
            "pokemon_hp": get_pokemon_hp(),
            "event_distribution": get_event_distribution(),
            "survival": get_battle_survival()[:20],
        }
    }
