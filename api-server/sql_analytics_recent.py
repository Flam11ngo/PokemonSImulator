"""
SQL analytics for the RECENT database (last hour only).
Same queries as sql_analytics but reading from pokemon_recent.db.
"""
import sqlite3, time, threading
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
DB_PATH = PROJECT_ROOT / "data" / "output_recent.db"

_tls = threading.local()

def _db():
    if not hasattr(_tls, 'conn') or _tls.conn is None:
        _tls.conn = sqlite3.connect(str(DB_PATH))
        _tls.conn.row_factory = sqlite3.Row
        _tls.conn.execute("PRAGMA journal_mode=WAL")
        _tls.conn.execute("PRAGMA cache_size=-8000")
    return _tls.conn

def _has_data():
    return _db().execute("SELECT COUNT(*) FROM battle_pokemon_states").fetchone()[0] > 0

def is_available():
    return _has_data()

# ── Same queries as sql_analytics, just from recent DB ──

def get_meta_species():
    return [dict(r) for r in _db().execute("""
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
    """).fetchall()]

def get_meta_moves():
    return [dict(r) for r in _db().execute("""
        SELECT CAST(v.value AS INTEGER) as move_id,
               COUNT(DISTINCT bps.battle_id||'-'||bps.species_id) as times_seen
        FROM battle_pokemon_states bps, json_each(bps.move_ids) v
        GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()]

def get_meta_items():
    return [dict(r) for r in _db().execute("""
        SELECT item_id,
               COUNT(DISTINCT battle_id||'-'||species_id) as uses,
               ROUND(AVG(hp_pct),1) as avg_hp
        FROM battle_pokemon_states WHERE item_id>0
        GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()]

def get_meta_abilities():
    return [dict(r) for r in _db().execute("""
        SELECT ability_id,
               COUNT(DISTINCT battle_id||'-'||species_id) as uses
        FROM battle_pokemon_states WHERE ability_id>0
        GROUP BY 1 ORDER BY 2 DESC
    """).fetchall()]
