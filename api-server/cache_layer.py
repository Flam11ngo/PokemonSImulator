"""Cache layer: Redis if available, otherwise in-memory dict with JSON persistence."""
import json, os
from pathlib import Path

REDIS_AVAILABLE = False
_redis = None
_store = {}
_store_path = Path(__file__).resolve().parent.parent / "data" / "cache.json"

# ── Try Redis ──
try:
    import redis as _r
    _redis = _r.Redis(host=os.getenv("REDIS_HOST", "127.0.0.1"),
                      port=int(os.getenv("REDIS_PORT", "6379")),
                      db=0, socket_connect_timeout=2)
    _redis.ping()
    REDIS_AVAILABLE = True
except Exception:
    # Fallback: load persisted JSON cache
    try:
        if _store_path.exists():
            _store = json.loads(_store_path.read_text(encoding="utf-8"))
    except Exception:
        _store = {}

def _save_fallback():
    try: _store_path.write_text(json.dumps(_store, ensure_ascii=False), encoding="utf-8")
    except: pass

# ── Public API ──

def get(key: str, default=None):
    if REDIS_AVAILABLE:
        val = _redis.get(key)
        return json.loads(val) if val else default
    return _store.get(key, default)

def set(key: str, value, ex=None):
    if REDIS_AVAILABLE:
        _redis.set(key, json.dumps(value, ensure_ascii=False), ex=ex)
    else:
        _store[key] = value
        _save_fallback()

def delete(key: str):
    if REDIS_AVAILABLE:
        _redis.delete(key)
    else:
        _store.pop(key, None)
        _save_fallback()

def cache_species_list(data: list):
    """Cache full species list (1025 entries). Keyed by id and by name prefix."""
    for s in data:
        sid = str(s["id"])
        set(f"sp:{sid}", s)
    # Search index: first 2 chars → list of ids
    from collections import defaultdict
    idx = defaultdict(list)
    for s in data:
        prefix = s["name"][:2].lower()
        idx[prefix].append(s["id"])
    for prefix, ids in idx.items():
        set(f"sp_idx:{prefix}", ids)
    set("sp:all", data)

def search_species(query: str, limit=15):
    """Fuzzy search species by name, cached."""
    if not query:
        keys = [_redis.keys("sp:*")] if REDIS_AVAILABLE else [f"sp:{k}" for k in _store if k.startswith("sp:") and k != "sp:all" and not k.startswith("sp_idx:")]
    prefix = query[:2].lower()
    ids = get(f"sp_idx:{prefix}", [])
    results = []
    for sid in ids[:limit * 2]:
        s = get(f"sp:{sid}")
        if s and _fuzzy_match(query, s["name"]):
            results.append(s)
        if len(results) >= limit:
            break
    return results[:limit]

def _fuzzy_match(q: str, name: str) -> bool:
    q, n, qi = q.lower(), name.lower(), 0
    for c in n:
        if qi < len(q) and c == q[qi]: qi += 1
    return qi == len(q)
