"""
Stats API — reads ADS summary tables only (pre-computed by bridge every 10s).
Zero raw event scans. Instant frontend responses.
"""
import sqlite3, json, os, re
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs, unquote

DB_PATH = "/opt/bigdata/pokemon_stats.db"
GAME_DB = "/opt/bigdata/pokemon.db"
SMOGON_DB = os.path.expanduser("~/temp/gen91v1_stats.sqlite")
PORT = int(os.environ.get("STATS_PORT", "8080"))

_name_cache = {"species": {}, "moves": {}, "items": {}, "abilities": {}}
_smogon_cn = {}  # english_name → chinese_name for Smogon data
_loaded = False

def load_names():
    global _loaded
    if _loaded: return
    try:
        g = sqlite3.connect(GAME_DB)
        for row in g.execute("SELECT id, name FROM species"): _name_cache["species"][str(row[0])] = row[1]
        for row in g.execute("SELECT id, name FROM moves"): _name_cache["moves"][str(row[0])] = row[1]
        for row in g.execute("SELECT id, name FROM items"): _name_cache["items"][str(row[0])] = row[1]
        for row in g.execute("SELECT id, name FROM abilities"): _name_cache["abilities"][str(row[0])] = row[1]
        # Smogon Chinese: english→chinese from name_mapping + species name
        for row in g.execute("SELECT english, chinese FROM name_mapping WHERE chinese != ''"):
            _smogon_cn[row[0].lower().strip()] = row[1]
        for row in g.execute("SELECT name, chinese_name FROM moves WHERE chinese_name != ''"):
            _smogon_cn[row[0].lower().strip()] = row[1]
        for row in g.execute("SELECT name, chinese_name FROM items WHERE chinese_name != ''"):
            _smogon_cn[row[0].lower().strip()] = row[1]
        for row in g.execute("SELECT name, chinese_name FROM abilities WHERE chinese_name != ''"):
            _smogon_cn[row[0].lower().strip()] = row[1]
        g.close()
        _loaded = True
    except Exception as e: print("Name load error:", e)

def sname(sid): return _name_cache["species"].get(str(sid), str(sid))
def mname(mid): return _name_cache["moves"].get(str(mid), str(mid))
def iname(iid): return _name_cache["items"].get(str(iid), str(iid))
def aname(aid): return _name_cache["abilities"].get(str(aid), str(aid))

def smogon_cn(name):
    """Lookup Chinese name for a Smogon English name."""
    if not name: return ""
    return _smogon_cn.get(name.lower().strip(), "")

def get_db():
    conn = sqlite3.connect(DB_PATH, timeout=10)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    conn.execute("PRAGMA busy_timeout=5000")
    conn.row_factory = sqlite3.Row
    return conn

def query(sql, params=()):
    for attempt in range(3):
        try:
            conn = get_db()
            rows = [dict(r) for r in conn.execute(sql, params)]
            conn.close()
            return rows
        except Exception as e:
            if attempt < 2:
                import time; time.sleep(0.5)
            else:
                print(f"query error after 3 retries: {e}")
    return []

class Handler(BaseHTTPRequestHandler):
    def _ok(self, data):
        body = json.dumps({"ok": True, "data": data}, ensure_ascii=False).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers(); self.wfile.write(body)

    def _err(self, msg, code=500):
        body = json.dumps({"ok": False, "error": msg}).encode()
        self.send_response(code); self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers(); self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?")[0]
        try:
            load_names()
            # ── Full snapshot — single request, all data ──
            if path == "/api/v1/stats/snapshot":
                load_names()
                ss = {r["key"]: r["value"] for r in query("SELECT key, value FROM summary_stats")}
                sp = query("SELECT name, usage_pct, appearance_count FROM meta_species ORDER BY appearance_count DESC LIMIT 50")
                for r in sp: r["species_name"] = sname(r["name"]); r["species_id"] = r["name"]
                mv = query("SELECT name, usage_pct FROM meta_moves ORDER BY usage_pct DESC LIMIT 50")
                for r in mv: r["move_name"] = mname(r["name"]); r["move_id"] = r["name"]
                it = query("SELECT name, usage_pct FROM meta_items ORDER BY usage_pct DESC LIMIT 30")
                for r in it: r["item_name"] = iname(r["name"]); r["item_id"] = r["name"]
                ab = query("SELECT name, usage_pct FROM meta_abilities ORDER BY usage_pct DESC LIMIT 30")
                for r in ab: r["ability_name"] = aname(r["name"]); r["ability_id"] = r["name"]
                pv = query("SELECT page, visit_count as n FROM page_dwell ORDER BY visit_count DESC")
                pd = query("SELECT page, total_dwell_seconds, visit_count FROM page_dwell ORDER BY total_dwell_seconds DESC")
                cl = query("SELECT element, count as n FROM click_stats ORDER BY n DESC LIMIT 20")
                ps = query("SELECT player_id, events, last_seen FROM player_stats ORDER BY events DESC LIMIT 20")
                rc = query("SELECT id, ts as timestamp, player_id, event, detail FROM recent_events ORDER BY id DESC LIMIT 50")
                ev = query("SELECT event_type, count FROM event_counts ORDER BY count DESC")

                # Types from pokemon.db
                g = sqlite3.connect(GAME_DB, timeout=3); g.row_factory = sqlite3.Row
                type_counts = {}
                for row in g.execute("SELECT type1 as tn, COUNT(*) as cnt FROM species WHERE type1 IS NOT NULL AND type1 != '' GROUP BY type1"):
                    type_counts[row["tn"]] = type_counts.get(row["tn"], 0) + row["cnt"]
                for row in g.execute("SELECT type2 as tn, COUNT(*) as cnt FROM species WHERE type2 IS NOT NULL AND type2 != '' GROUP BY type2"):
                    type_counts[row["tn"]] = type_counts.get(row["tn"], 0) + row["cnt"]
                g.close()
                tp = [{"type_id": k, "type_name": k, "appearances": v, "species_count": v} for k, v in sorted(type_counts.items(), key=lambda x: -x[1])]

                return self._ok({
                    "summary": {"species": len(sp), "battles": int(ss.get("battles", 0)), "events": int(ss.get("total_events", 0)), "players": int(ss.get("players", 0))},
                    "species": sp, "moves": mv, "items": it, "abilities": ab, "types": tp,
                    "page_views": pv, "page_dwell": pd, "clicks": cl, "players": ps, "recent": rc, "event_counts": ev,
                })

            # ── Health ──
            elif path == "/health":
                return self._ok({"db": os.path.exists(DB_PATH)})

            # ── Deep summary (from summary_stats) ──
            elif path == "/api/v1/stats/deep/summary":
                ss = {r["key"]: r["value"] for r in query("SELECT key, value FROM summary_stats")}
                sp = len(query("SELECT DISTINCT name FROM meta_species"))
                ev = ss.get("total_events", 0)
                return self._ok({"species": sp, "battles": int(ss.get("battles", 0)), "events": int(ev), "turns": 0})

            # ── Species (from meta_species summary) ──
            elif path == "/api/v1/stats/deep/meta":
                rows = query("SELECT name, usage_pct, appearance_count FROM meta_species ORDER BY appearance_count DESC LIMIT 50")
                for r in rows:
                    r["species_name"] = sname(r["name"]); r["species_id"] = r["name"]
                return self._ok(rows)

            # ── Moves (from meta_moves summary) ──
            elif path == "/api/v1/stats/deep/moves":
                rows = query("SELECT name, usage_pct FROM meta_moves ORDER BY usage_pct DESC LIMIT 50")
                for r in rows:
                    r["move_name"] = mname(r["name"]); r["move_id"] = r["name"]
                return self._ok(rows)

            # ── Items (from meta_items summary) ──
            elif path == "/api/v1/stats/deep/items":
                rows = query("SELECT name, usage_pct FROM meta_items ORDER BY usage_pct DESC LIMIT 30")
                for r in rows:
                    r["item_name"] = iname(r["name"]); r["item_id"] = r["name"]
                return self._ok(rows)

            # ── Abilities (from meta_abilities summary) ──
            elif path == "/api/v1/stats/deep/abilities":
                rows = query("SELECT name, usage_pct FROM meta_abilities ORDER BY usage_pct DESC LIMIT 30")
                for r in rows:
                    r["ability_name"] = aname(r["name"]); r["ability_id"] = r["name"]
                return self._ok(rows)

            # ── Types (from pokemon.db — small, always fast) ──
            elif path == "/api/v1/stats/deep/types":
                g = sqlite3.connect(GAME_DB, timeout=3); g.row_factory = sqlite3.Row
                type_counts = {}
                for row in g.execute("SELECT type1 as tn, COUNT(*) as cnt FROM species WHERE type1 IS NOT NULL AND type1 != '' GROUP BY type1"):
                    type_counts[row["tn"]] = type_counts.get(row["tn"], 0) + row["cnt"]
                for row in g.execute("SELECT type2 as tn, COUNT(*) as cnt FROM species WHERE type2 IS NOT NULL AND type2 != '' GROUP BY type2"):
                    type_counts[row["tn"]] = type_counts.get(row["tn"], 0) + row["cnt"]
                g.close()
                result = [{"type_id": k, "type_name": k, "appearances": v, "species_count": v} for k, v in sorted(type_counts.items(), key=lambda x: -x[1])]
                return self._ok(result)

            # ── Events (from event_counts summary) ──
            elif path == "/api/v1/stats/deep/events":
                return self._ok(query("SELECT event_type, count FROM event_counts ORDER BY count DESC"))

            # ── Live ──
            elif path == "/api/v1/stats/deep/live":
                sp = query("SELECT name, usage_pct, appearance_count FROM meta_species ORDER BY appearance_count DESC LIMIT 30")
                for r in sp: r["species_name"] = sname(r["name"]); r["species_id"] = r["name"]
                mv = query("SELECT name, usage_pct FROM meta_moves ORDER BY usage_pct DESC LIMIT 30")
                for r in mv: r["move_name"] = mname(r["name"]); r["move_id"] = r["name"]
                return self._ok({"species_usage": sp, "move_usage": mv})

            # ── All ──
            elif path == "/api/v1/stats/deep/all":
                sp = query("SELECT name, usage_pct, appearance_count FROM meta_species ORDER BY appearance_count DESC LIMIT 50")
                for r in sp: r["species_name"] = sname(r["name"]); r["species_id"] = r["name"]
                mv = query("SELECT name, usage_pct FROM meta_moves ORDER BY usage_pct DESC LIMIT 30")
                for r in mv: r["move_name"] = mname(r["name"]); r["move_id"] = r["name"]
                it = query("SELECT name, usage_pct FROM meta_items ORDER BY usage_pct DESC LIMIT 30")
                for r in it: r["item_name"] = iname(r["name"]); r["item_id"] = r["name"]
                ab = query("SELECT name, usage_pct FROM meta_abilities ORDER BY usage_pct DESC LIMIT 30")
                for r in ab: r["ability_name"] = aname(r["name"]); r["ability_id"] = r["name"]
                return self._ok({"species": sp, "moves": mv, "items": it, "abilities": ab})

            # ── Global ──
            elif path == "/api/v1/stats/global":
                ss = {r["key"]: r["value"] for r in query("SELECT key, value FROM summary_stats")}
                return self._ok({"total_battles": int(ss.get("battles", 0)), "total_completed_battles": int(ss.get("battles", 0)),
                                 "total_players": int(ss.get("players", 0)), "total_teams": 0, "species_count": 0, "events_count": 0})

            # ═══════════════════════════════════════
            # UI Analytics — all from summary tables
            # ═══════════════════════════════════════

            elif path == "/api/v1/stats/ui/summary":
                ss = {r["key"]: r["value"] for r in query("SELECT key, value FROM summary_stats")}
                pv = query("SELECT page, visit_count as n FROM page_dwell ORDER BY visit_count DESC")
                return self._ok({"players": int(ss.get("players", 0)), "total_events": int(ss.get("total_events", 0)), "page_views": pv})

            elif path == "/api/v1/stats/ui/clicks":
                return self._ok(query("SELECT element, count as n FROM click_stats ORDER BY n DESC LIMIT 20"))

            elif path == "/api/v1/stats/ui/players":
                return self._ok(query("SELECT player_id, events, last_seen FROM player_stats ORDER BY events DESC LIMIT 20"))

            elif path == "/api/v1/stats/ui/recent":
                return self._ok(query("SELECT id, ts as timestamp, player_id, event, detail FROM recent_events ORDER BY id DESC LIMIT 50"))

            elif path == "/api/v1/stats/ui/page_dwell":
                return self._ok(query("SELECT page, total_dwell_seconds, visit_count FROM page_dwell ORDER BY total_dwell_seconds DESC"))

            elif path == "/api/v1/stats/ui/favorites":
                rows = query("SELECT element, count FROM click_stats ORDER BY count DESC LIMIT 15")
                tf = [{"feature": r["element"], "score": r["count"]} for r in rows]
                return self._ok({"top_features": tf, "player_favorites": []})

            # ═══ Smogon endpoints — read from ~/temp/gen91v1_stats.sqlite ═══
            elif path == "/api/v1/smogon/filters":
                return self._smogon_filters()
            elif path == "/api/v1/smogon/pokemon":
                return self._smogon_ranking()
            elif path.startswith("/api/v1/smogon/pokemon/"):
                name = unquote(path.split("/api/v1/smogon/pokemon/")[1])
                return self._smogon_detail(name)
            elif path == "/api/v1/smogon/summary":
                return self._smogon_summary()
            elif path == "/api/v1/smogon/moves":
                return self._smogon_moves()
            elif path == "/api/v1/smogon/items":
                return self._smogon_items()
            elif path.startswith("/api/v1/smogon/trend/"):
                name = unquote(path.split("/api/v1/smogon/trend/")[1])
                return self._smogon_trend(name)

            else:
                return self._err(f"Unknown: {path}", 404)
        except Exception as e:
            return self._err(str(e))

    # ═══ Smogon query methods ═══
    def _add_cn(self, item, table):
        """Add chinese_name field to a Smogon result item."""
        key = item.get("name") or item.get("mate") or item.get("opp") or item.get("type") or ""
        item["chinese_name"] = smogon_cn(key)
        return item

    def _smogon_open(self):
        conn = sqlite3.connect(SMOGON_DB, timeout=5)
        conn.row_factory = sqlite3.Row
        return conn

    def _smogon_filters(self):
        try:
            conn = self._smogon_open()
            sources = [r[0] for r in conn.execute("SELECT DISTINCT source FROM mon ORDER BY source")]
            tbs = [r[0] for r in conn.execute("SELECT DISTINCT time_bucket FROM mon ORDER BY time_bucket DESC")]
            ratings = [r[0] for r in conn.execute("SELECT DISTINCT rating FROM mon ORDER BY rating")]
            conn.close()
            return self._ok({"sources": sources, "time_buckets": tbs, "ratings": ratings})
        except Exception as e:
            return self._err(str(e))

    def _smogon_ranking(self):
        try:
            qs = parse_qs(urlparse(self.path).query)
            source = qs.get("source", ["smogon"])[0]
            tb = qs.get("time_bucket", ["2026-05"])[0]
            rating = int(qs.get("rating", [1760])[0])
            limit = int(qs.get("limit", [50])[0])
            offset = int(qs.get("offset", [0])[0])
            search = qs.get("search", [None])[0]

            conn = self._smogon_open()
            if search:
                rows = conn.execute(
                    "SELECT name, usage, viability_ceiling FROM mon WHERE source=? AND time_bucket=? AND rating=? AND name LIKE ? ORDER BY usage DESC LIMIT ? OFFSET ?",
                    (source, tb, rating, f"%{search}%", limit, offset)
                ).fetchall()
            else:
                rows = conn.execute(
                    "SELECT name, usage, viability_ceiling FROM mon WHERE source=? AND time_bucket=? AND rating=? ORDER BY usage DESC LIMIT ? OFFSET ?",
                    (source, tb, rating, limit, offset)
                ).fetchall()
            conn.close()
            results = [dict(r) for r in rows]
            for r in results: r["chinese_name"] = smogon_cn(r.get("name",""))
            return self._ok(results)
        except Exception as e:
            return self._err(str(e))

    def _smogon_detail(self, name):
        try:
            qs = parse_qs(urlparse(self.path).query)
            source = qs.get("source", ["smogon"])[0]
            tb = qs.get("time_bucket", ["2026-05"])[0]
            rating = int(qs.get("rating", [1760])[0])

            conn = self._smogon_open()
            info_row = conn.execute(
                "SELECT name, usage, viability_ceiling FROM mon WHERE name=? AND source=? AND time_bucket=? AND rating=?",
                (name, source, tb, rating)
            ).fetchone()
            if not info_row:
                conn.close()
                return self._err(f"Pokemon '{name}' not found", 404)

            info = dict(info_row)
            info["chinese_name"] = smogon_cn(info.get("name",""))

            def fetch_rel(table, select="name, usage", order="usage DESC", limit=15):
                return [dict(r) for r in conn.execute(
                    f"SELECT {select} FROM {table} WHERE mon=? AND source=? AND time_bucket=? AND rating=? ORDER BY {order} LIMIT ?",
                    (name, source, tb, rating, limit)
                ).fetchall()]

            # CC fallback: retry with rating=1500/0 if empty
            def fetch_cc(limit=15):
                for r in [rating, 1500, 0]:
                    rows = [dict(rr) for rr in conn.execute(
                        "SELECT opp, percentage, stddev FROM cc WHERE mon=? AND source=? AND time_bucket=? AND rating=? ORDER BY percentage DESC LIMIT ?",
                        (name, source, tb, r, limit)
                    ).fetchall()]
                    if rows: return rows
                return []
            ccs = fetch_cc()

            result = {
                "info": info,
                "abilities": [self._add_cn(a, "abilities") for a in fetch_rel("ability")],
                "moves": [self._add_cn(m, "moves") for m in fetch_rel("move")],
                "items": [self._add_cn(it, "items") for it in fetch_rel("item")],
                "teras": fetch_rel("tera", select="type, usage"),
                "spreads": fetch_rel("spread", select="nature, evs, usage"),
                "teammates": [self._add_cn(t, "species") for t in fetch_rel("team", select="mate, usage")],
                "ccs": [self._add_cn(c, "species") for c in ccs],
            }
            conn.close()
            return self._ok(result)
        except Exception as e:
            return self._err(str(e))

    def _smogon_summary(self):
        try:
            qs = parse_qs(urlparse(self.path).query)
            source = qs.get("source", ["smogon"])[0]
            tb = qs.get("time_bucket", ["2026-05"])[0]
            rating = int(qs.get("rating", [1760])[0])

            conn = self._smogon_open()
            total = conn.execute("SELECT COUNT(DISTINCT name) FROM mon WHERE source=? AND time_bucket=? AND rating=?", (source, tb, rating)).fetchone()[0]
            avg_row = conn.execute("SELECT ROUND(AVG(usage)*100,2), ROUND(MAX(usage)*100,2) FROM mon WHERE source=? AND time_bucket=? AND rating=?", (source, tb, rating)).fetchone()
            # Top item/ability/move
            top_item = conn.execute("SELECT name FROM item WHERE source=? AND time_bucket=? AND rating=? GROUP BY name ORDER BY COUNT(DISTINCT mon) DESC LIMIT 1", (source, tb, rating)).fetchone()
            top_ab = conn.execute("SELECT name FROM ability WHERE source=? AND time_bucket=? AND rating=? GROUP BY name ORDER BY COUNT(DISTINCT mon) DESC LIMIT 1", (source, tb, rating)).fetchone()
            top_mv = conn.execute("SELECT name FROM move WHERE source=? AND time_bucket=? AND rating=? GROUP BY name ORDER BY COUNT(DISTINCT mon) DESC LIMIT 1", (source, tb, rating)).fetchone()
            conn.close()

            return self._ok({
                "total": total,
                "avg_usage_pct": avg_row[0],
                "max_usage_pct": avg_row[1],
                "top_item": top_item[0] if top_item else None,
                "top_ability": top_ab[0] if top_ab else None,
                "top_move": top_mv[0] if top_mv else None,
            })
        except Exception as e:
            return self._err(str(e))

    def _smogon_moves(self):
        try:
            qs = parse_qs(urlparse(self.path).query)
            source = qs.get("source", ["smogon"])[0]
            tb = qs.get("time_bucket", ["2026-05"])[0]
            rating = int(qs.get("rating", [1760])[0])
            limit = int(qs.get("limit", [20])[0])

            conn = self._smogon_open()
            rows = [dict(r) for r in conn.execute(
                "SELECT name, COUNT(DISTINCT mon) as mon_count FROM move WHERE source=? AND time_bucket=? AND rating=? GROUP BY name ORDER BY mon_count DESC LIMIT ?",
                (source, tb, rating, limit)
            ).fetchall()]
            conn.close()
            return self._ok(rows)
        except Exception as e:
            return self._err(str(e))

    def _smogon_items(self):
        try:
            qs = parse_qs(urlparse(self.path).query)
            source = qs.get("source", ["smogon"])[0]
            tb = qs.get("time_bucket", ["2026-05"])[0]
            rating = int(qs.get("rating", [1760])[0])
            limit = int(qs.get("limit", [10])[0])

            conn = self._smogon_open()
            items = [dict(r) for r in conn.execute(
                "SELECT name, COUNT(DISTINCT mon) as mon_count FROM item WHERE source=? AND time_bucket=? AND rating=? GROUP BY name ORDER BY mon_count DESC LIMIT ?",
                (source, tb, rating, limit)
            ).fetchall()]
            abilities = [dict(r) for r in conn.execute(
                "SELECT name, COUNT(DISTINCT mon) as mon_count FROM ability WHERE source=? AND time_bucket=? AND rating=? GROUP BY name ORDER BY mon_count DESC LIMIT ?",
                (source, tb, rating, limit)
            ).fetchall()]
            conn.close()
            for it in items: it["chinese_name"] = smogon_cn(it.get("name",""))
            for a in abilities: a["chinese_name"] = smogon_cn(a.get("name",""))
            return self._ok({"items": items, "abilities": abilities})
        except Exception as e:
            return self._err(str(e))

    def _smogon_trend(self, name):
        try:
            qs = parse_qs(urlparse(self.path).query)
            rating = int(qs.get("rating", [1760])[0])
            source = qs.get("source", ["smogon"])[0]

            conn = self._smogon_open()
            rows = [dict(r) for r in conn.execute(
                "SELECT time_bucket, ROUND(usage*100,2) as usage_pct, ROUND(viability_ceiling,1) as vc FROM mon WHERE name=? AND source=? AND rating=? ORDER BY time_bucket",
                (name, source, rating)
            ).fetchall()]
            conn.close()
            return self._ok(rows)
        except Exception as e:
            return self._err(str(e))

def main():
    load_names()
    print(f"Stats API: 0.0.0.0:{PORT} (ADS summary mode)")
    HTTPServer(("0.0.0.0", PORT), Handler).serve_forever()

if __name__ == "__main__":
    main()
