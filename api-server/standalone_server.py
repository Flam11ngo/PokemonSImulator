from __future__ import annotations
"""
PokemonSimulator Standalone Server — WebSocket mode.
One WebSocket connection handles all communication (battles, data, matchmaking).
"""
import asyncio, json, os, sys, uuid, subprocess, tempfile, shutil, logging, threading, platform, sqlite3
from pathlib import Path
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Request
from fastapi.middleware.cors import CORSMiddleware
import uvicorn

logging.basicConfig(level=logging.INFO, format='%(asctime)s [ws] %(levelname)s: %(message)s')
logger = logging.getLogger("ws-server")

app = FastAPI(title="PokemonSimulator WebSocket API", version="3.0.0")
app.add_middleware(CORSMiddleware, allow_origins=["*"], allow_credentials=True,
                   allow_methods=["*"], allow_headers=["*"])

IS_WINDOWS = platform.system() == "Windows"
PROJECT_ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(PROJECT_ROOT / "smogon_stats"))

# Engine adapter — Node.js Showdown engine (drop-in replacement for C++ daemon)
ENGINE_ADAPTER = PROJECT_ROOT / "engine-adapter" / "showdown_daemon.js"
ENGINE_CMD = ["node", str(ENGINE_ADAPTER)]
DATA_DIR = PROJECT_ROOT / "data"

# ============================================================
# Game Data Cache
# ============================================================
_species_list: list = []          # base-form species only (no Mega/Gmax/regional), for search/display
_species_all: list = []           # all species including forms, for battle lookups
_species_by_id: dict = {}         # id → first entry (base form)
_item_form_map: dict = {}         # (item_id, species_id) → {alias_name, sprite_name}
_moves: dict = {}
_abilities: dict = {}
_items: dict = {}

def load_all_data():
    """Load species/moves/abilities/items from SQLite database (pokemon.db)."""
    global _species_list, _species_all, _species_by_id, _moves, _abilities, _items
    db_path = DATA_DIR / "pokemon.db"
    if not db_path.exists():
        logger.warning(f"pokemon.db not found at {db_path}. Run: python scripts/py/rebuild_db.py")
        return
    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row

    # ── Moves ──
    for row in conn.execute("SELECT * FROM moves"):
        _moves[row["id"]] = {
            "id": row["id"], "name": row["name"],
            "chineseName": row["chinese_name"] or "",
            "type": row["type"], "category": row["category"] or "",
            "power": row["power"] or 0, "accuracy": row["accuracy"] or 0,
            "pp": row["pp"] or 0, "description": row["description"] or "",
            "chineseDesc": row["chinese_desc"] or ""}

    # ── Abilities ──
    for row in conn.execute("SELECT * FROM abilities"):
        _abilities[row["id"]] = {"id": row["id"], "name": row["name"],
            "chineseName": row["chinese_name"] or ""}

    # ── Items ──
    for row in conn.execute("SELECT * FROM items"):
        _items[row["id"]] = {"id": row["id"], "name": row["name"],
            "chineseName": row["chinese_name"] or "", "description": row["description"] or ""}

    # ── Species (base forms from species table) ──
    _species_list = []
    _species_all = []
    _species_by_id = {}

    # Pre-load all learnsets into memory for fast access
    learnset_map = {}
    for row in conn.execute("SELECT species_id, move_id FROM learnsets ORDER BY species_id"):
        learnset_map.setdefault(row["species_id"], []).append(row["move_id"])

    # Pre-load all species abilities
    ability_map = {}  # species_id → [(ability_id, is_hidden), ...]
    for row in conn.execute("SELECT * FROM species_abilities"):
        ability_map.setdefault(row["species_id"], []).append(
            (row["ability_id"], row["is_hidden"]))

    # ── Chinese name mappings ──
    _cn_names = {}
    for row in conn.execute("SELECT english, chinese FROM name_mapping"):
        _cn_names[row["english"]] = row["chinese"]

    for row in conn.execute("SELECT * FROM species WHERE id > 0 ORDER BY id"):
        sid = row["id"]
        abilities_list = []
        hidden_id = 0
        for aid, is_hidden in ability_map.get(sid, []):
            if is_hidden:
                hidden_id = aid
            else:
                if aid not in abilities_list:
                    abilities_list.append(aid)

        entry = {
            "id": sid, "name": row["name"],
            "chineseName": _cn_names.get(row["name"], ""),
            "types": [t for t in [row["type1"], row["type2"]] if t],
            "baseStats": [row["base_hp"], row["base_atk"], row["base_def"],
                          row["base_spa"], row["base_spd"], row["base_spe"]],
            "abilities": abilities_list,
            "hiddenAbilityID": hidden_id or row["hidden_ability"] or 0,
            "learnableMoves": learnset_map.get(sid, []),
            "abilityNames": [_abilities[aid]["name"] for aid in abilities_list if aid in _abilities],
            "hiddenAbilityName": _abilities[hidden_id]["name"] if hidden_id and hidden_id in _abilities else "",
        }
        _species_list.append(entry)
        _species_all.append(entry)
        if sid not in _species_by_id:
            _species_by_id[sid] = entry

    # ── Species aliases (form mappings) ──
    alias_count = 0
    for row in conn.execute("SELECT * FROM species_aliases WHERE show_in_list IS NULL OR show_in_list = 1"):
        base_sid = row["species_id"]
        base = _species_by_id.get(base_sid)
        if not base: continue
        alias_id = base_sid + 100000  # virtual ID for form variants
        entry = {
            "id": alias_id,
            "name": row["alias_name"],
            "chineseName": _cn_names.get(row["alias_name"], base.get("chineseName", "")),
            "spriteName": row["sprite_name"],
            "baseSpeciesId": base_sid,
            "types": base["types"],
            "baseStats": base["baseStats"],
            "abilities": base["abilities"],
            "hiddenAbilityID": base["hiddenAbilityID"],
            "learnableMoves": base["learnableMoves"],
            "abilityNames": base["abilityNames"],
            "hiddenAbilityName": base["hiddenAbilityName"],
        }
        _species_list.append(entry)
        _species_all.append(entry)
        _species_by_id[alias_id] = entry
        alias_count += 1

    # ── Item → Form mapping (for battle sprite switching) ──
    global _item_form_map
    _item_form_map.clear()
    for row in conn.execute("SELECT * FROM item_form_map"):
        key = (row["item_id"], row["species_id"])
        _item_form_map[key] = {"alias_name": row["alias_name"], "sprite_name": row["sprite_name"]}

    # Load ALL aliases into _species_by_id for battle sprite lookup (even hidden ones)
    for row in conn.execute("SELECT * FROM species_aliases"):
        base = _species_by_id.get(row["species_id"])
        if not base: continue
        alias_id = row["species_id"] + 100000
        if alias_id not in _species_by_id:
            _species_by_id[alias_id] = {**base, "id": alias_id,
                "name": row["alias_name"], "chineseName": _cn_names.get(row["alias_name"], base.get("chineseName", "")),
                "spriteName": row["sprite_name"], "baseSpeciesId": row["species_id"]}

    conn.close()
    logger.info(f"Loaded {len(_species_list)} species ({alias_count} form aliases visible, {len(_item_form_map)} item-form mappings), {len(_moves)} moves, {len(_abilities)} abilities, {len(_items)} items from SQLite")

load_all_data()

def to_showdown_item(item_id: int) -> int:
    """Item IDs are now Showdown-native — identity mapping (was PokeAPI->Showdown bridge)."""
    return item_id

def convert_team_items(team: dict) -> dict:
    """Convert team item IDs (now Showdown-native, kept for compatibility)."""
    for p in team.get("pokemon", []):
        if p.get("item"):
            p["item"] = to_showdown_item(p["item"])
    return team

def fuzzy_match(query: str, name: str) -> bool:
    """Characters-in-order fuzzy match. 'pik' matches 'Pikachu', 'chr' matches 'Charizard'."""
    q = query.lower(); n = name.lower(); qi = 0
    for c in n:
        if qi < len(q) and c == q[qi]: qi += 1
    return qi == len(q)

ENUMS = {"type": [
    {"value":0,"name":"Normal","label":"一般"},{"value":1,"name":"Fire","label":"火"},
    {"value":2,"name":"Water","label":"水"},{"value":3,"name":"Electric","label":"电"},
    {"value":4,"name":"Grass","label":"草"},{"value":5,"name":"Ice","label":"冰"},
    {"value":6,"name":"Fighting","label":"格斗"},{"value":7,"name":"Poison","label":"毒"},
    {"value":8,"name":"Ground","label":"地面"},{"value":9,"name":"Flying","label":"飞行"},
    {"value":10,"name":"Psychic","label":"超能力"},{"value":11,"name":"Bug","label":"虫"},
    {"value":12,"name":"Rock","label":"岩石"},{"value":13,"name":"Ghost","label":"幽灵"},
    {"value":14,"name":"Dragon","label":"龙"},{"value":15,"name":"Dark","label":"恶"},
    {"value":16,"name":"Steel","label":"钢"},{"value":17,"name":"Fairy","label":"妖精"}],
    "category": [{"value":0,"name":"Physical","label":"物理"},{"value":1,"name":"Special","label":"特殊"},{"value":2,"name":"Status","label":"变化"}],
    "weather": [{"value":0,"name":"Clear","label":"无天气"},{"value":1,"name":"Rain","label":"雨天"},{"value":2,"name":"Sun","label":"晴天"},{"value":3,"name":"Sandstorm","label":"沙暴"},{"value":4,"name":"Hail","label":"冰雹"},{"value":5,"name":"Snow","label":"雪天"}],
    "status": [{"value":0,"name":"None","label":"无"},{"value":1,"name":"Burn","label":"灼伤"},{"value":2,"name":"Freeze","label":"冰冻"},{"value":3,"name":"Paralysis","label":"麻痹"},{"value":4,"name":"Poison","label":"中毒"},{"value":5,"name":"Sleep","label":"睡眠"},{"value":7,"name":"ToxicPoison","label":"剧毒"},{"value":8,"name":"Confusion","label":"混乱"}],
}

# ============================================================
# Battle Daemon — long-running engine subprocess per battle
# ============================================================
import time, threading

class BattleEngine:
    """Wraps C++ --daemon mode. Files pre-written, filesystem polling only."""

    def __init__(self, battle_id, init_json):
        self.battle_id = battle_id
        self.init_json = init_json
        self.turn = 0
        self.ended = False
        self.work_dir = Path(tempfile.mkdtemp(prefix=f"battle_{battle_id}_"))
        self.turns = {}
        self._lock = threading.Lock()
        self._proc = None

        # Setup (must create output dir too)
        (self.work_dir / "cache" / "input").mkdir(parents=True)
        (self.work_dir / "cache" / "output").mkdir(parents=True)

        # Write init files BEFORE starting daemon
        side_a = init_json.get("side_a", {})
        side_b = init_json.get("side_b", {})
        with open(self.work_dir / "cache/input/side_a.json", "w") as f: json.dump(side_a, f)
        with open(self.work_dir / "cache/input/side_b.json", "w") as f: json.dump(side_b, f)
        with open(self.work_dir / "cache/input/1_input_0.json", "w") as f: json.dump({"type": "pass"}, f)
        with open(self.work_dir / "cache/input/2_input_0.json", "w") as f: json.dump({"type": "pass"}, f)

        # Start Showdown engine adapter (Node.js, no C++ dependencies)
        self._log = open(self.work_dir / "daemon.log", "a")
        self._proc = subprocess.Popen(
            ENGINE_CMD + [str(self.work_dir)],
            stdout=self._log, stderr=self._log
        )
        logger.info(f"[Engine:{battle_id}] Showdown daemon started, waiting for turn 0...")
        self._wait_output(0, timeout=30)
        logger.info(f"[Engine:{battle_id}] Turn 0 ready")

    def _wait_output(self, turn_num, timeout=30, suffix=""):
        """Poll for output_N.json or output_N_force.json on filesystem."""
        f = self.work_dir / "cache" / "output" / f"output_{turn_num}{suffix}.json"
        logger.info(f"[Engine:{self.battle_id}] _wait_output: polling {f}")
        dl = time.time() + timeout
        while time.time() < dl:
            if f.exists():
                st = f.stat()
                if st.st_size > 0:
                    time.sleep(0.02)
                    with open(f, encoding='utf-8') as fh: return enrich_events(json.load(fh))
                else:
                    logger.warning(f"[Engine:{self.battle_id}] output file exists but empty: {f}")
            if self._proc and self._proc.poll() is not None:
                rc = self._proc.returncode
                # Check daemon log for errors
                dlog = self.work_dir / "daemon.log"
                tail = ""
                if dlog.exists():
                    lines = dlog.read_text(encoding='utf-8').split('\n')[-5:]
                    tail = "\n  daemon tail: " + " | ".join(lines)
                raise RuntimeError(f"Engine exited (code {rc}) at turn {turn_num}{suffix}{tail}")
            time.sleep(0.1)
        # List files before timeout for debugging
        out_files = list((self.work_dir / "cache" / "output").glob("*.json"))
        logger.error(f"[Engine:{self.battle_id}] TIMEOUT waiting {f.name}. Output files: {[x.name for x in out_files]}")
        raise TimeoutError(f"Timeout waiting output_{turn_num}{suffix}")

    def get_state(self):
        f = self.work_dir / "cache" / "output" / f"output_{self.turn}.json"
        if f.exists():
            with open(f, encoding='utf-8') as fh: return enrich_events(json.load(fh))
        return {"turn": 0, "battle": {"sides": [
            {"name": self.init_json.get("side_a",{}).get("name","A"), "pokemons": [], "active": 0},
            {"name": self.init_json.get("side_b",{}).get("name","B"), "pokemons": [], "active": 0}]}}

    def write_force_switch_file(self, side, switch_index):
        """Write force switch input file WITHOUT waiting for output.
        Used when both sides need to switch — write both files, then wait once."""
        with self._lock:
            prefix = "1" if side in ("a", "A") else "2"
            tfile = self.work_dir / f"cache/input/{prefix}_input_{self.turn}_force.json"
            logger.info(f"[Engine:{self.battle_id}] write_force_switch_file: side={side} idx={switch_index} file={tfile.name}")
            with open(tfile, "w") as f:
                json.dump({"type": "switch", "switch_index": switch_index}, f)

    def process_force_switch(self, side, switch_index):
        """Process a forced switch after KO. Writes _force suffix file and waits for output."""
        self.write_force_switch_file(side, switch_index)
        with self._lock:
            logger.info(f"[Engine:{self.battle_id}] waiting force switch output (turn {self.turn})")
            result = self._wait_output(self.turn, timeout=15, suffix="_force")
            if result:
                self.turns[self.turn] = self.turns.get(self.turn, []) + [{"type": "force_switch", "side": side}]
            return result

    def process_turn(self, actions):
        with self._lock:
            if self.ended: return None
            nt = len(self.turns) + 1
            self.turns[nt] = actions
            for act in actions:
                clean = {k: v for k, v in act.items() if v is not None}
                side = clean.pop("side", "").lower()
                pfx = "1" if side in ("a", "side_a", "player_a") else "2"
                fpath = self.work_dir / f"cache/input/{pfx}_input_{nt}.json"
                with open(fpath, "w") as f: json.dump(clean, f)
                logger.info(f"[Engine:{self.battle_id}] wrote {fpath.name}: {clean}")
            logger.info(f"[Engine:{self.battle_id}] waiting turn {nt} output (files in input: {list((self.work_dir/'cache'/'input').glob('*.json'))})")
            result = self._wait_output(nt, timeout=30)
            self.turn = nt
            for sd in result.get("battle", result).get("sides", []):
                if sd.get("pokemons") and all(p.get("fainted") for p in sd["pokemons"]):
                    self.ended = True; break
            return result

    def cleanup(self):
        try:
            if self._proc and self._proc.poll() is None:
                self._proc.terminate(); self._proc.wait(timeout=3)
        except: pass
        shutil.rmtree(self.work_dir, ignore_errors=True)
# ============================================================

WEATHER_NAMES = {0:"无天气",1:"雨天☔",2:"晴天☀️",3:"沙暴🏜️",4:"冰雹🌨️",5:"雪天❄️"}
FIELD_NAMES = {0:"无场地",1:"精神场地🔮",2:"电气场地⚡",3:"青草场地🌿",4:"薄雾场地🌫️",5:"戏法空间🔄"}

def enrich_events(data):
    battle = data.get("battle", data)
    # Add species names + detect need2switch (faint → force switch)
    pending_sides = []
    for sd in battle.get("sides", []):
        if sd.get("need2switch") or sd.get("_pendingSwitch"):
            pending_sides.append(sd.get("name", "?"))
        for p in sd.get("pokemons", []):
            sid = p.get("speciesId", 0)
            if sid and sid in _species_by_id:
                sp = _species_by_id[sid]
                p["_speciesName"] = sp["name"]
                # Check item → form mapping for sprite switching
                item_id = p.get("item") or p.get("itemId") or 0
                form_key = (item_id, sid)
                if form_key in _item_form_map:
                    fm = _item_form_map[form_key]
                    p["_speciesName"] = fm["alias_name"]
                    p["_formSpriteId"] = sid + 100000
                    p["_formSpriteName"] = fm["sprite_name"]
                    logger.info(f"[form] {sp['name']} + item#{item_id} → {fm['alias_name']}")
                elif p.get("details"):
                    # Fallback: parse details string e.g. "Ogerpon-Wellspring, L50, F"
                    detail_name = p["details"].split(",")[0].strip()
                    if detail_name != sp["name"]:
                        for alias_sid, entry in _species_by_id.items():
                            if alias_sid >= 100000 and entry["name"] == detail_name:
                                p["_speciesName"] = detail_name
                                p["_formSpriteId"] = alias_sid
                                p["_formSpriteName"] = entry.get("spriteName", "")
                                logger.info(f"[form] detail match: {detail_name} → sprite #{alias_sid}")
                                break
    data["_hasPendingSwitch"] = len(pending_sides) > 0

    # Weather/field — keep adapter's labels if present, otherwise use maps
    w = data.get("_weather", {}) or {}
    f = data.get("_field", {}) or {}
    if not w.get("label") and w.get("type"):
        w["label"] = WEATHER_NAMES.get(w["type"], "")
        data["_weather"] = w
    if not f.get("label") and f.get("type"):
        f["label"] = FIELD_NAMES.get(f["type"], "")
        data["_field"] = f

    # Daemon now provides structured event_type + side. Map p1/p2 → a/b
    for ev in data.get("events", []):
        if ev.get("side") == "p1": ev["side"] = "a"
        elif ev.get("side") == "p2": ev["side"] = "b"
    return data

# ============================================================
# In-memory state
# ============================================================
battles_db: dict = {}     # battle_id → battle metadata
battle_engines: dict = {} # battle_id → BattleEngine instance
match_pool: list = []
match_battles: dict = {}  # player_id → {battle_id, side}
match_pending: dict = {}  # battle_id → {side: action}
bot_battles: dict = {}    # battle_id → True (vs-bot flag)
connections: dict[str, WebSocket] = {}
pending_matches: dict = {}  # battle_id → {p1_id, p2_id, state} — engine init in progress

# ============================================================
# WebSocket Endpoint
# ============================================================
@app.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    await ws.accept()
    player_id = None
    logger.info("WebSocket connected")

    try:
        while True:
            raw = await ws.receive_text()
            msg = json.loads(raw)
            msg_type = msg.get("type", "")
            data = msg.get("data", {})
            req_id = msg.get("id", None)  # optional request tracking

            try:
                if msg_type == "handshake":
                    player_id = data.get("player_id", str(uuid.uuid4())[:8])
                    connections[player_id] = ws
                    await send(ws, "handshake_ok", {"player_id": player_id})
                    # Check for pending matches — deliver immediately if engine is ready
                    for bid, pm in list(pending_matches.items()):
                        if player_id == pm["p1_id"] and pm["state"]:
                            await send(ws, "matched", {"battle_id": bid, "side": "a", "state": pm["state"]})
                            logger.info(f"Delivered pending match {bid} to reconnected p1={player_id}")
                        elif player_id == pm["p2_id"] and pm["state"]:
                            await send(ws, "matched", {"battle_id": bid, "side": "b", "state": pm["state"]})
                            logger.info(f"Delivered pending match {bid} to reconnected p2={player_id}")

                # --- Battles ---
                elif msg_type == "create_battle":
                    bid = str(uuid.uuid4())[:8]
                    team_a = json.loads(data["team_a_json"])
                    team_b = json.loads(data["team_b_json"])
                    seed = data.get("seed", 0)
                    init_json = {"side_a": convert_team_items(team_a), "side_b": convert_team_items(team_b), "seed": seed}
                    loop = asyncio.get_running_loop()
                    daemon = await loop.run_in_executor(None, lambda: BattleEngine(bid, init_json))
                    battle_engines[bid] = daemon
                    state = daemon.get_state()
                    battle = {"id": bid, "status": "active", "total_turns": 0,
                              "init_json": json.dumps(init_json), "current_state": state, "turns": []}
                    battles_db[bid] = battle
                    battle["turns"].append({"turn": 0, "state": state})
                    # Build form alias lookup: name → (virtual_id, sprite_name)
                    form_aliases = {}
                    for sid, entry in _species_by_id.items():
                        if sid >= 100000 and entry.get("spriteName"):
                            form_aliases[entry["name"]] = {"sid": sid, "spriteName": entry["spriteName"]}
                    await send(ws, "battle_created", {
                        "battle": battle, "state": state,
                        "itemFormMap": {str(k[0])+"_"+str(k[1]): v for k, v in _item_form_map.items()},
                        "formAliases": form_aliases,
                    })

                elif msg_type == "process_turn":
                    battle_id = data["battle_id"]
                    daemon = battle_engines.get(battle_id)
                    battle = battles_db.get(battle_id)
                    if not daemon or not battle:
                        await send(ws, "error", {"message": "Battle not found"}, req_id); continue
                    loop = asyncio.get_running_loop()
                    result = await loop.run_in_executor(None, lambda: daemon.process_turn(data["actions"]))
                    if result is None:
                        await send(ws, "error", {"message": "Turn processing failed"}, req_id); continue
                    battle["total_turns"] = daemon.turn
                    battle["current_state"] = result
                    if daemon.ended:
                        battle["status"] = "completed"
                        daemon.cleanup()
                    await send(ws, "turn_processed", {"battle_id": battle_id, "turn": daemon.turn,
                                "status": battle["status"], "state": result})

                elif msg_type == "get_battle":
                    b = battles_db.get(data["battle_id"])
                    await send(ws, "battle_data", b or {})

                elif msg_type == "get_battles":
                    blist = list(battles_db.values())
                    blist.sort(key=lambda x: str(x.get("id","")), reverse=True)
                    await send(ws, "battles_list", blist[:50])

                # --- Matchmaking ---
                elif msg_type == "join_matchmaking":
                    pid = data["player_id"]
                    team_json = data["team_json"]
                    opponent_type = data.get("opponent_type", "human")  # "human" or "bot"
                    global match_pool, bot_battles

                    # ---- VS BOT: create battle immediately ----
                    if opponent_type == "bot":
                        import random as _random
                        # Pick a random bot team from available species
                        bot_team = {"pokemon": []}
                        all_ids = list(_species_by_id.keys())
                        if all_ids:
                            picks = _random.sample(all_ids, min(3, len(all_ids)))
                            for i, sid in enumerate(picks):
                                s = _species_by_id[sid]
                                # Get 4 random moves for this species
                                move_pool = s.get("learnableMoves", []) or [m["id"] for m in list(_moves.values())[:10]]
                                bot_moves = _random.sample(move_pool, min(4, len(move_pool))) if len(move_pool) >= 4 else (move_pool[:4] if move_pool else [1,2,3,4])
                                # All Showdown-mapped battle items (PokeAPI IDs, converted later)
                                _battle_items = [
                                    184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,
                                    201,202,203,204,205,206,207,208,209,210,211,  # berries + leftovers(211)
                                    213,214,217,219,220,221,222,225,230,232,233,234,235,236,237,238,239,
                                    240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
                                    265,266,267,268,269,270,271,272,273,274,275,276,277,278,279,280,
                                    281,282,283,284,285,286,287,288,
                                ]
                                bot_item = _random.choice(_battle_items)
                                bot_team["pokemon"].append({
                                    "speciesID": sid, "level": 50,
                                    "moves": bot_moves,
                                    "ability": s.get("hiddenAbilityID", 0) or _random.choice(s.get("abilities", [0]) or [0]),
                                    "nature": _random.randint(0, 24),
                                    "item": bot_item, "evs": {}, "ivs": {}
                                })
                        # Build init_json with human vs bot
                        player_team = convert_team_items(json.loads(team_json))
                        init_json = {"side_a": player_team, "side_b": bot_team, "seed": 42}
                        bid = str(uuid.uuid4())[:8]
                        bot_battles[bid] = True
                        p1_id, p2_id = pid, f"bot_{bid}"
                        logger.info(f"Bot battle! {pid} vs {p2_id} → {bid}")

                        # Register pending + match info
                        pending_matches[bid] = {"p1_id": p1_id, "p2_id": p2_id, "state": None, "init_json": init_json}
                        match_battles[p1_id] = {"battle_id": bid, "side": "a"}
                        match_battles[p2_id] = {"battle_id": bid, "side": "b"}

                        # Start engine in background
                        async def start_bot_engine(bid, p1_id, p2_id, init_json):
                            try:
                                loop = asyncio.get_running_loop()
                                daemon = await loop.run_in_executor(None, lambda: BattleEngine(bid, init_json))
                                battle_engines[bid] = daemon; bot_battles[bid] = True
                                state = daemon.get_state()
                                battle = {"id": bid, "player_a_id": p1_id, "player_b_id": p2_id,
                                          "seed": 42, "status": "active", "total_turns": 0,
                                          "init_json": json.dumps(init_json), "current_state": state, "turns": [{"turn": 0, "state": state}]}
                                battles_db[bid] = battle
                                pending_matches[bid]["state"] = state
                                w = connections.get(p1_id)
                                if w: await send(w, "matched", {"battle_id": bid, "side": "a", "state": state})
                                pending_matches.pop(bid, None)
                                logger.info(f"Bot battle {bid} ready")
                            except Exception as e:
                                logger.error(f"Bot battle init failed {bid}: {e}", exc_info=True)
                                pending_matches.pop(bid, None)
                        asyncio.create_task(start_bot_engine(bid, p1_id, p2_id, init_json))
                        continue

                    # ---- HUMAN vs HUMAN: normal matchmaking ----
                    # Check if already in pool
                    already = any(p["player_id"] == pid for p in match_pool)
                    if already:
                        await send(ws, "matchmaking_status", {"status": "already_queued", "pool_size": len(match_pool)})
                        continue
                    match_pool.append({"player_id": pid, "team_json": team_json})
                    await send(ws, "matchmaking_status", {"status": "waiting", "pool_size": len(match_pool)})

                    if len(match_pool) >= 2:
                        p1 = match_pool.pop(0)
                        # Find a DIFFERENT player to match with (skip self-matches from duplicate tabs)
                        p2 = None
                        for i, p in enumerate(match_pool):
                            if p["player_id"] != p1["player_id"]:
                                p2 = match_pool.pop(i)
                                break
                        if p2 is None:
                            # Only same user in pool, put p1 back
                            match_pool.insert(0, p1)
                            continue
                        bid = str(uuid.uuid4())[:8]
                        p1_id, p2_id = p1["player_id"], p2["player_id"]
                        team_a, team_b = json.loads(p1["team_json"]), json.loads(p2["team_json"])
                        init_json = {"side_a": team_a, "side_b": team_b, "seed": 42}
                        logger.info(f"Match! {p1_id} vs {p2_id} → {bid}, starting engine...")

                        # Register pending match BEFORE engine init (so reconnecting players find it)
                        pending_matches[bid] = {"p1_id": p1_id, "p2_id": p2_id, "state": None,
                                                 "init_json": init_json}
                        # Tell both players immediately — don't make them wait 7s in silence
                        w1_now, w2_now = connections.get(p1_id), connections.get(p2_id)
                        if w1_now: await send(w1_now, "match_found", {"battle_id": bid, "status": "starting"})
                        if w2_now: await send(w2_now, "match_found", {"battle_id": bid, "status": "starting"})

                        # Start engine in background (don't block matching handler)
                        async def start_engine(bid, p1_id, p2_id, init_json):
                            try:
                                loop = asyncio.get_running_loop()
                                daemon = await loop.run_in_executor(None, lambda: BattleEngine(bid, init_json))
                                battle_engines[bid] = daemon
                                state = daemon.get_state()
                                logger.info(f"Match engine ready for {bid}")
                                battle = {"id": bid, "player_a_id": p1_id, "player_b_id": p2_id,
                                          "seed": 42, "status": "active", "total_turns": 0,
                                          "init_json": json.dumps(init_json), "current_state": state}
                                battles_db[bid] = battle
                                match_battles[p1_id] = {"battle_id": bid, "side": "a"}
                                match_battles[p2_id] = {"battle_id": bid, "side": "b"}
                                # Update pending with state (for reconnecting players)
                                pending_matches[bid]["state"] = state
                                # Send matched with retries (handle transient disconnects)
                                for attempt in range(10):
                                    w1, w2 = connections.get(p1_id), connections.get(p2_id)
                                    if w1 and w2:
                                        logger.info(f"Sending matched: p1={p1_id} ws=OK, p2={p2_id} ws=OK")
                                        await send(w1, "matched", {"battle_id": bid, "side": "a", "state": state})
                                        await send(w2, "matched", {"battle_id": bid, "side": "b", "state": state})
                                        logger.info(f"Matched messages sent for {bid}")
                                        pending_matches.pop(bid, None)  # done
                                        return
                                    missing = []
                                    if not w1: missing.append(p1_id)
                                    if not w2: missing.append(p2_id)
                                    logger.info(f"Retry {attempt+1}/10 for {bid}: waiting for {missing}")
                                    await asyncio.sleep(1.5)
                                # Timeout — clean up
                                logger.error(f"Match {bid} timed out waiting for players")
                                pending_matches.pop(bid, None)
                                if bid in battle_engines:
                                    battle_engines[bid].cleanup()
                                    del battle_engines[bid]
                            except Exception as e:
                                logger.error(f"Engine init failed for {bid}: {e}", exc_info=True)
                                pending_matches.pop(bid, None)

                        asyncio.create_task(start_engine(bid, p1_id, p2_id, init_json))

                elif msg_type == "force_switch":
                    battle_id = data["battle_id"]
                    daemon = battle_engines.get(battle_id)
                    if not daemon:
                        await send(ws, "error", {"message": "Battle not found"}, req_id); continue
                    side = data.get("side", "a")
                    switch_index = data.get("switch_index", 0)

                    # ---- VS BOT: ensure bot force file is written (don't wait) ----
                    if bot_battles.get(battle_id):
                        bot_side = "b" if side == "a" else "a"
                        st = daemon.get_state()
                        bot_s = st.get("battle", {}).get("sides", [])[0 if bot_side == "a" else 1]
                        if bot_s.get("need2switch"):
                            bot_idx = 0
                            for i, p in enumerate(bot_s.get("pokemons", [])):
                                if not p.get("fainted") and i != bot_s.get("active", -1):
                                    bot_idx = i; break
                            logger.info(f"Bot force switch: side={bot_side} idx={bot_idx}")
                            loop = asyncio.get_running_loop()
                            # write-only (no wait) — may already exist from submit_action
                            await loop.run_in_executor(None, lambda: daemon.write_force_switch_file(bot_side, bot_idx))

                    # Process human force switch (writes file + waits for daemon output)
                    loop = asyncio.get_running_loop()
                    state = await loop.run_in_executor(None, lambda: daemon.process_force_switch(side, switch_index))
                    battle = battles_db.get(battle_id)
                    if battle:
                        battle["current_state"] = state or battle.get("current_state")
                        if state:
                            battle.setdefault("turns", []).append({"turn": daemon.turn, "state": state, "phase": "force_switch"})
                    # Push to both players
                    for pid_s, info in match_battles.items():
                        if info.get("battle_id") == battle_id:
                            w = connections.get(pid_s)
                            if w: await send(w, "turn_processed", {"battle_id": battle_id,
                                "turn": daemon.turn, "status": battle["status"] if battle else "active", "state": state or battle["current_state"]})
                    await send(ws, "force_switch_done", {"state": state}, req_id)

                elif msg_type == "submit_action":
                    battle_id = data["battle_id"]
                    logger.info(f"[submit_action] received: battle={battle_id} action={data.get('action',{})}")
                    battle = battles_db.get(battle_id)
                    daemon = battle_engines.get(battle_id)
                    if not battle or not daemon:
                        logger.error(f"[submit_action] battle/daemon NOT FOUND: {battle_id}")
                        await send(ws, "error", {"message": "Battle not found"}, req_id); continue
                    action = data["action"]
                    side = action.get("side", "")
                    clean = {k: v for k, v in action.items() if v is not None and k != "side"}
                    pending = match_pending.setdefault(battle_id, {})
                    pending[side] = clean
                    logger.info(f"[submit_action] pending: {list(pending.keys())} len={len(pending)}")

                    # ---- VS BOT: auto-fill bot action ----
                    if bot_battles.get(battle_id) and len(pending) == 1:
                        bot_side = "b" if side == "a" else "a"
                        logger.info(f"Bot auto-fill triggered: pending={list(pending.keys())} bot_side={bot_side}")
                        # Send feedback before auto-fill so player knows action was received
                        await send(ws, "action_submitted", {"side": side})
                        try:
                            state = daemon.get_state()
                            bot_s = state["battle"]["sides"][0 if bot_side == "a" else 1]
                            active = bot_s["pokemons"][bot_s.get("active", 0)]
                            logger.info(f"Bot active: {active.get('_speciesName','?')} hp={active.get('hp')} moves={[(m.get('id'),m.get('pp')) for m in active.get('moves',[])]}")
                            bot_idx = 0
                            for i, m in enumerate(active.get("moves", [])):
                                if m.get("pp", 0) > 0:
                                    bot_idx = i
                                    break
                            pending[bot_side] = {"type": "attack", "move_index": bot_idx}
                            logger.info(f"Bot auto-pick: side={bot_side} move_idx={bot_idx} move_id={active.get('moves',[])[bot_idx].get('id') if active.get('moves') else '?'}")
                        except Exception as e:
                            logger.warning(f"Bot auto-pick failed, using pass: {e}")
                            pending[bot_side] = {"type": "pass"}

                    if len(pending) >= 2:
                        actions_list = [{"side": s, **pending[s]} for s in ["a","b"] if s in pending]
                        loop = asyncio.get_running_loop()
                        result = await loop.run_in_executor(None, lambda: daemon.process_turn(actions_list))
                        match_pending.pop(battle_id, None)
                        battle["total_turns"] = daemon.turn
                        battle["current_state"] = result or battle.get("current_state")
                        battle.setdefault("turns", []).append({"turn": daemon.turn, "state": result or battle.get("current_state")})
                        if daemon.ended:
                            battle["status"] = "completed"
                            daemon.cleanup()

                        # After turn: handle bot auto-force-switch and merge events
                        if bot_battles.get(battle_id) and not daemon.ended:
                            st = result or daemon.get_state()
                            sides_st = st.get("battle", {}).get("sides", [])
                            human_s = sides_st[0 if side == "a" else 1] if len(sides_st) > 1 else {}
                            bot_s   = sides_st[0 if side != "a" else 1] if len(sides_st) > 1 else {}
                            bot_side_tag = "b" if side == "a" else "a"
                            if bot_s.get("need2switch"):
                                bot_idx = 0
                                for i, p in enumerate(bot_s.get("pokemons", [])):
                                    if not p.get("fainted") and i != bot_s.get("active", -1):
                                        bot_idx = i; break
                                if not human_s.get("need2switch"):
                                    # Only bot fainted: auto-switch and merge events
                                    logger.info(f"Bot auto-force-switch (bot-only): side={bot_side_tag} idx={bot_idx}")
                                    loop2 = asyncio.get_running_loop()
                                    result2 = await loop2.run_in_executor(None, lambda: daemon.process_force_switch(bot_side_tag, bot_idx))
                                    if result2:
                                        # Merge: keep turn events, append force switch events
                                        turn_events = (result or {}).get("events", []) or []
                                        fs_events = result2.get("events", []) or []
                                        result2["events"] = turn_events + fs_events
                                        battle["current_state"] = result2
                                        result = result2
                                        logger.info(f"[submit_action] merged events: {len(turn_events)} turn + {len(fs_events)} force-switch")
                                else:
                                    # Both fainted: write bot file now, human's force_switch handler will finish
                                    logger.info(f"Bot auto-force-switch (both faint): side={bot_side_tag} idx={bot_idx}")
                                    loop2 = asyncio.get_running_loop()
                                    await loop2.run_in_executor(None, lambda: daemon.write_force_switch_file(bot_side_tag, bot_idx))

                        logger.info(f"[submit_action] turn_processed: turn={daemon.turn} events={len((result or {}).get('events',[]))} sending to {list(match_battles.keys())}")
                        for pid_s, info in match_battles.items():
                            if info.get("battle_id") == battle_id:
                                w = connections.get(pid_s)
                                if w:
                                    await send(w, "turn_processed", {"battle_id": battle_id,
                                        "turn": daemon.turn, "status": battle["status"],
                                        "state": result or battle.get("current_state")})
                                    logger.info(f"[submit_action] sent turn_processed to {pid_s}")
                                else:
                                    # Buffer the state for when player reconnects
                                    logger.warning(f"[submit_action] {pid_s} not connected, buffering state")
                                    battle["pending_state"] = result or battle.get("current_state")
                        continue
                    else:
                        await send(ws, "action_submitted", {"side": side})

                # --- Data ---
                elif msg_type == "get_species":
                    q = data.get("search", "").strip()
                    limit = int(data.get("limit", 15))
                    if q:
                        results = [s for s in _species_list if fuzzy_match(q, s["name"])][:limit]
                    else:
                        results = _species_list[:limit]
                    await send(ws, "species_list", results, req_id)

                elif msg_type == "get_moves":
                    q = data.get("search", "").strip()
                    limit = data.get("limit", 15)
                    learnset = data.get("learnset", [])  # optional: list of allowed move IDs
                    # Build candidate pool (full or learnset-filtered)
                    if learnset:
                        pool = [_moves[i] for i in learnset if i in _moves]
                    else:
                        pool = list(_moves.values())
                    if q:
                        results = [m for m in pool if fuzzy_match(q, m["name"])][:limit]
                    else:
                        results = pool[:limit]
                    await send(ws, "moves_list", results, req_id)

                elif msg_type == "get_move":
                    mv = _moves.get(data["id"])
                    await send(ws, "move_data", mv or {}, req_id)

                elif msg_type == "get_ability":
                    ab = _abilities.get(data["id"])
                    await send(ws, "ability_data", ab or {}, req_id)

                elif msg_type == "get_abilities":
                    q = data.get("search", "").strip()
                    if q:
                        results = [a for a in _abilities.values() if fuzzy_match(q, a["name"])][:15]
                    else:
                        common = [9,66,3,26,168,45,146,67,10]
                        results = [_abilities[i] for i in common if i in _abilities]
                    await send(ws, "abilities_list", results, req_id)

                elif msg_type == "get_items":
                    q = data.get("search", "").strip()
                    limit = int(data.get("limit", 300))
                    # Keep only battle-holdable items: held items with combat effects + berries
                    skip_kw = ['ball','-ball','stone','-stone','mail','fossil',
                               'shard','nugget','pearl','shoal',
                               'rare-candy','rare bone','big-mushroom','balm-mushroom',
                               'tiny-mushroom','stardust','star-piece','comet-shard',
                               'pp-up','pp-max','heart-scale','honey',
                               'growth-','stable-','gooey-','damp-','heat-','smooth-','icy-',
                               'ball','-ball','ball-','stone','-stone','mail',
                               'fossil','-fossil','fossil-',
                               'mega','-ite','ite','ium-z','ium z','z-crystal',
                               'tr00','tr01','tr02','tr03','tr04','tr05','tr06','tr07','tr08','tr09',
                               'tr10','tr11','tr12','tr13','tr14','tr15','tr16','tr17','tr18','tr19',
                               'tr20','tr30','tr40','tr50','tr60','tr70','tr80','tr90',
                               '-tm','hm','data-card','coupon',
                               'key','ticket','pass','letter','parcel','scope',
                               'bike','rod','flute','case','pouch','kit','sack',
                               'album','goods','chip','capsule','card','candy',
                               'mushroom','bone','pearl','nugget','stardust','comet',
                               'vitamin','protein','iron','calcium','zinc','carbos',
                               'hp-up','pp-up','pp-max','rare-candy','exp-share',
                               'mulch','apricorn','doll','toy','contest','scarf',
                               'sweet-apple','tart-apple','syrupy-apple',
                               'cracked-pot','chipped-pot','whipped-dream','sachet',
                               'galarica','malicious-armor','auspicious-armor',
                               'teacup','metal-alloy',
                               'deep-sea-tooth','deep-sea-scale',
                               'electirizer','magmarizer','protector','reaper-cloth',
                               'up-grade','dubious-disc','prism-scale',
                               'dragon-scale','kings-rock','metal-coat',
                               'bottle-cap','gold-bottle-cap',
                               'macho-brace','power-bracer','power-belt','power-lens',
                               'power-band','power-anklet','power-weight',
                               '-drive','drive-','memory',
                               'poke-doll','poke-toy','fluffy-tail',
                               'old-gateau','lava-cookie','fresh-water','soda-pop',
                               'lemonade','moomoo-milk','energy-root','heal-powder',
                               'revival-herb','sacred-ash','full-restore','full-heal',
                               'antidote','burn-heal','ice-heal','awakening',
                               'escape-rope','max-mushrooms','berry-juice',
                               'sweet-heart','rare bone','shoal-salt','shoal-shell',
                               'red-orb','blue-orb','jade-orb',
                               'clear-bell','tidal-bell','blue-card','red-scale',
                               'lucky-punch','lucky-egg','amulet-coin',
                               'silver-wing','rainbow-wing','soothe-bell','cleanse-tag',
                               'vile-vial','crucibellite','fairy-feather']
                    _whitelist = ['choice scarf','red card','safety goggles','snowball','scope lens',
                                  'silver powder','silk scarf','iron ball','light ball','smoke ball',
                                  'big root','black sludge']
                    def is_battle_item(it):
                        name = it.get('name','').lower().replace(' ','-')
                        if it.get('name','').lower().strip() in _whitelist: return True
                        for kw in skip_kw:
                            if kw in name: return False
                        return True
                    battle_items = [i for i in _items.values() if is_battle_item(i)]
                    if q:
                        results = [i for i in battle_items if fuzzy_match(q, i["name"])][:limit]
                    else:
                        results = battle_items[:limit]
                    await send(ws, "items_list", results, req_id)

                elif msg_type == "get_sprite_url":
                    sid = data.get("id", 0)
                    sp = _species_by_id.get(sid, {})
                    name = sp.get("name", "").lower().replace(" ", "").replace(".", "").replace("'", "").replace("-", "")
                    # Pokemon Showdown CDN (fast, optimized for sprites)
                    url = f"https://play.pokemonshowdown.com/sprites/ani/{name}.gif" if name else ""
                    await send(ws, "sprite_url", {"id": sid, "url": url}, req_id)

                elif msg_type == "get_enums":
                    await send(ws, "enums", ENUMS, req_id)

                elif msg_type == "get_global_stats":
                    await send(ws, "global_stats", {
                        "total_battles": len(battles_db),
                        "completed": sum(1 for b in battles_db.values() if b.get("status")=="completed"),
                        "match_pool": len(match_pool),
                    }, req_id)

                # --- Users & Teams (SQLite-backed) ---
                elif msg_type == "save_user":
                    import sqlite3
                    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
                    name = data.get("name", "Trainer")
                    conn.execute("INSERT OR IGNORE INTO users (username) VALUES (?)", (name,))
                    conn.commit(); conn.close()
                    await send(ws, "user_saved", {"username": name}, req_id)

                elif msg_type == "save_team":
                    import sqlite3
                    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
                    username = data.get("user_id", data.get("username", ""))
                    team_name = data.get("name", "Team")
                    team_json = json.dumps(data.get("pokemon", []))
                    conn.execute("INSERT OR IGNORE INTO users (username) VALUES (?)", (username,))
                    conn.execute("INSERT OR REPLACE INTO user_teams (username,team_name,team_json,updated_at) VALUES (?,?,?,datetime('now'))",
                                 (username, team_name, team_json))
                    conn.commit(); conn.close()
                    await send(ws, "team_saved", {"team_name": team_name}, req_id)

                elif msg_type == "delete_team":
                    import sqlite3
                    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
                    username = data.get("username", "")
                    team_name = data.get("team_name", "")
                    conn.execute("DELETE FROM user_teams WHERE username=? AND team_name=?", (username, team_name))
                    conn.commit(); conn.close()
                    await send(ws, "team_deleted", {"team_name": team_name}, req_id)

                elif msg_type == "get_user_teams":
                    import sqlite3
                    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
                    username = data.get("user_id", data.get("username", ""))
                    rows = conn.execute("SELECT * FROM user_teams WHERE username=? ORDER BY updated_at DESC", (username,)).fetchall()
                    conn.close()
                    teams = [{"name": r[1], "pokemon": json.loads(r[2])} for r in rows]
                    await send(ws, "user_teams", teams, req_id)

                # --- Health ---
                elif msg_type == "quit_battle":
                    # Player explicitly quits a battle — notify opponent and clean up
                    battle_id = data.get("battle_id")
                    daemon = battle_engines.pop(battle_id, None) if battle_id else None
                    if daemon:
                        try: daemon.cleanup()
                        except: pass
                    battles_db.pop(battle_id, None) if battle_id else None
                    # Notify opponent
                    for pid_s, info in list(match_battles.items()):
                        if info.get("battle_id") == battle_id and pid_s != player_id:
                            w = connections.get(pid_s)
                            if w:
                                try:
                                    await send(w, "opponent_disconnected",
                                               {"battle_id": battle_id, "message": "对手退出了对战"})
                                except: pass
                            match_battles.pop(pid_s, None)
                    match_battles.pop(player_id, None)
                    await send(ws, "battle_quit", {"battle_id": battle_id}, req_id)

                elif msg_type == "get_current_battle":
                    # Restore battle state when player navigates back to matchmaking page
                    binfo = match_battles.get(player_id)
                    if binfo:
                        battle = battles_db.get(binfo["battle_id"])
                        daemon = battle_engines.get(binfo["battle_id"])
                        if battle and daemon:
                            state = daemon.get_state()
                            await send(ws, "matched", {
                                "battle_id": binfo["battle_id"],
                                "side": binfo["side"],
                                "state": state
                            })
                            logger.info(f"Restored battle {binfo['battle_id']} for reconnected {player_id}")
                        else:
                            await send(ws, "no_active_battle", {})
                    else:
                        await send(ws, "no_active_battle", {})

                elif msg_type == "ping":
                    await send(ws, "pong", {"engine": ENGINE_ADAPTER.exists(), "species": len(_species_list)})

                elif msg_type == "analytics_batch":
                    _store_analytics(data.get("events", []))

                else:
                    await send(ws, "error", {"message": f"Unknown type: {msg_type}"}, req_id)

            except Exception as e:
                logger.error(f"Handler error: {e}", exc_info=True)
                await send(ws, "error", {"message": str(e)}, req_id)

    except WebSocketDisconnect:
        logger.info(f"WebSocket disconnected: {player_id}")
    except Exception as e:
        logger.error(f"WebSocket error: {e}")
    finally:
        if player_id:
            connections.pop(player_id, None)
            # Remove from matchmaking pool (match_pool is module-level, reassign modifies it)
            match_pool[:] = [p for p in match_pool if p["player_id"] != player_id]
            # ---- Clean up active battle ----
            battle_info = match_battles.get(player_id)
            if battle_info:
                battle_id = battle_info["battle_id"]
                match_battles.pop(player_id, None)
                match_pending.pop(battle_id, None)
                bot_battles.pop(battle_id, None)
                pending_matches.pop(battle_id, None)
                # Find opponent
                opponent_id = None
                for pid_s, info in list(match_battles.items()):
                    if info.get("battle_id") == battle_id:
                        opponent_id = pid_s
                        match_battles.pop(pid_s, None)
                        break
                if opponent_id and opponent_id in connections:
                    try:
                        await send(connections[opponent_id], "opponent_disconnected",
                                   {"battle_id": battle_id, "message": "对手已断开连接"})
                    except: pass
                # Kill daemon
                daemon = battle_engines.pop(battle_id, None)
                if daemon:
                    try: daemon.cleanup()
                    except: pass
                battles_db.pop(battle_id, None)
                logger.info(f"Battle {battle_id} cleaned up ({player_id} disconnected)")

            # Remove from match pool
            match_pool[:] = [p for p in match_pool if p["player_id"] != player_id]
            # Clean up pending matches
            for bid, pm in list(pending_matches.items()):
                if player_id in (pm.get("p1_id"), pm.get("p2_id")):
                    # Notify the other pending player
                    other_id = pm["p2_id"] if player_id == pm.get("p1_id") else pm.get("p1_id")
                    if other_id and other_id in connections:
                        try:
                            await send(connections[other_id], "match_cancelled",
                                       {"message": "对手已离开"})
                        except: pass
                    pending_matches.pop(bid, None)
                    logger.info(f"Pending match {bid} cancelled ({player_id} disconnected)")
            # Remove from connections
            if player_id in connections:
                del connections[player_id]


async def send(ws: WebSocket, msg_type: str, data, req_id: str = None):
    msg = {"type": msg_type, "data": data}
    if req_id: msg["id"] = req_id
    await ws.send_json(msg)


# ============================================================
# Health (REST fallback)
# ============================================================
@app.get("/api/v1/health")
def health():
    return {"ok": True, "data": {"mode": "websocket", "species": len(_species_list),
            "moves": len(_moves), "abilities": len(_abilities), "engine": ENGINE_ADAPTER.exists()}}

# ============================================================
# REST API endpoints (for HistoryList, StatsDashboard, etc.)
# ============================================================

@app.get("/api/v1/health")
async def rest_health():
    return {"ok": True, "data": {"status": "healthy", "service": "pokemon-simulator-api", "version": "3.0.0"}}

@app.get("/api/v1/battles")
async def rest_list_battles(status: str = None, player_id: str = None, limit: int = 50, offset: int = 0):
    blist = list(battles_db.values())
    if status:
        blist = [b for b in blist if b.get("status") == status]
    blist.sort(key=lambda x: str(x.get("id","")), reverse=True)
    result = blist[offset:offset+limit]
    return {"ok": True, "data": result}

@app.get("/api/v1/battles/{battle_id}")
async def rest_get_battle(battle_id: str):
    b = battles_db.get(battle_id)
    if not b:
        return {"ok": False, "error": "Battle not found"}
    return {"ok": True, "data": b}

@app.get("/api/v1/players")
async def rest_list_players(limit: int = 50, offset: int = 0):
    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
    conn.row_factory = sqlite3.Row
    rows = conn.execute("SELECT username as name, created_at FROM users ORDER BY created_at DESC LIMIT ? OFFSET ?", (limit, offset)).fetchall()
    conn.close()
    return {"ok": True, "data": [dict(r) for r in rows]}

@app.get("/api/v1/players/{player_id}")
async def rest_get_player(player_id: str):
    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
    conn.row_factory = sqlite3.Row
    row = conn.execute("SELECT username as name, created_at FROM users WHERE username = ?", (player_id,)).fetchone()
    conn.close()
    if not row:
        return {"ok": False, "error": "Player not found"}
    return {"ok": True, "data": dict(row)}

@app.post("/api/v1/players")
async def rest_create_player(body: dict):
    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
    conn.execute("INSERT OR IGNORE INTO users (username) VALUES (?)", (body.get("name", ""),))
    conn.commit(); conn.close()
    return {"ok": True, "data": {"name": body.get("name", "")}}

@app.get("/api/v1/teams")
async def rest_list_teams(player_id: str = None, limit: int = 50, offset: int = 0):
    conn = sqlite3.connect(str(DATA_DIR / "pokemon.db"))
    conn.row_factory = sqlite3.Row
    if player_id:
        rows = conn.execute("SELECT username, team_name as name, team_json as pokemon, updated_at FROM user_teams WHERE username = ? ORDER BY updated_at DESC LIMIT ? OFFSET ?", (player_id, limit, offset)).fetchall()
    else:
        rows = conn.execute("SELECT username, team_name as name, team_json as pokemon, updated_at FROM user_teams ORDER BY updated_at DESC LIMIT ? OFFSET ?", (limit, offset)).fetchall()
    conn.close()
    result = []
    for r in rows:
        d = dict(r)
        d["pokemon"] = json.loads(d.get("pokemon", "[]"))
        result.append(d)
    return {"ok": True, "data": result}

@app.get("/api/v1/teams/{team_id}")
async def rest_get_team(team_id: str):
    return {"ok": False, "error": "Use player_id query param instead"}

@app.get("/api/v1/data/enums")
async def rest_get_enums():
    return {"ok": True, "data": ENUMS}

@app.get("/api/v1/stats/global")
async def rest_global_stats():
    return {"ok": True, "data": {
        "total_battles": len(battles_db),
        "total_completed_battles": sum(1 for b in battles_db.values() if b.get("status")=="completed"),
        "total_players": 0, "total_teams": 0
    }}

# ============================================================
# Smogon Stats API — big data dashboard endpoints
# ============================================================
SMOGON_DB = str(PROJECT_ROOT / "smogon_stats" / "gen91v1_stats.sqlite")

@app.get("/api/v1/smogon/filters")
async def smogon_filters():
    from queries import get_filters
    try:
        return {"ok": True, "data": get_filters(SMOGON_DB)}
    except Exception as e:
        return {"ok": False, "error": str(e)}

@app.get("/api/v1/smogon/pokemon")
async def smogon_pokemon(source: str, time_bucket: str, rating: int,
                          search: str = None, limit: int = 50, offset: int = 0):
    from queries import get_pokemon_ranking
    try:
        return {"ok": True, "data": get_pokemon_ranking(
            SMOGON_DB, source, time_bucket, rating, search, limit, offset)}
    except Exception as e:
        return {"ok": False, "error": str(e)}

@app.get("/api/v1/smogon/pokemon/{name}")
async def smogon_pokemon_detail(name: str, source: str, time_bucket: str, rating: int):
    from queries import get_pokemon_detail
    try:
        result = get_pokemon_detail(SMOGON_DB, name, source, time_bucket, rating)
        if result is None:
            return {"ok": False, "error": f"Pokemon '{name}' not found"}
        return {"ok": True, "data": result}
    except Exception as e:
        return {"ok": False, "error": str(e)}

@app.get("/api/v1/smogon/summary")
async def smogon_summary(source: str, time_bucket: str, rating: int):
    from queries import get_summary_stats
    try:
        return {"ok": True, "data": get_summary_stats(SMOGON_DB, source, time_bucket, rating)}
    except Exception as e:
        return {"ok": False, "error": str(e)}

@app.get("/api/v1/smogon/trend/{name}")
async def smogon_trend(name: str, rating: int = 1760, source: str = "smogon"):
    from queries import get_usage_trend
    try:
        return {"ok": True, "data": get_usage_trend(SMOGON_DB, name, source, rating)}
    except Exception as e:
        return {"ok": False, "error": str(e)}

@app.get("/api/v1/smogon/types")
async def smogon_types(source: str, time_bucket: str, rating: int, limit: int = 20):
    from queries import get_type_distribution
    try:
        return {"ok": True, "data": get_type_distribution(SMOGON_DB, source, time_bucket, rating, limit)}
    except Exception as e:
        return {"ok": False, "error": str(e)}

@app.get("/api/v1/smogon/items")
async def smogon_items(source: str, time_bucket: str, rating: int, limit: int = 10):
    from queries import get_top_items_abilities
    try:
        return {"ok": True, "data": get_top_items_abilities(SMOGON_DB, source, time_bucket, rating, limit)}
    except Exception as e:
        return {"ok": False, "error": str(e)}

@app.get("/api/v1/smogon/moves")
async def smogon_moves(source: str, time_bucket: str, rating: int, limit: int = 20):
    from queries import get_meta_moves
    try:
        return {"ok": True, "data": get_meta_moves(SMOGON_DB, source, time_bucket, rating, limit)}
    except Exception as e:
        return {"ok": False, "error": str(e)}

# ═══════════════════════════════════════════
# Analytics
# ═══════════════════════════════════════════

import threading
_analytics_lock = threading.Lock()
_analytics_buffer = []
_analytics_dir = PROJECT_ROOT / "logs" / "analytics"
_analytics_dir.mkdir(parents=True, exist_ok=True)

def _store_analytics(events):
    """Store analytics events. Writes to daily JSONL log file."""
    if not events:
        return
    with _analytics_lock:
        _analytics_buffer.extend(events)
        if len(_analytics_buffer) >= 50:
            _flush_analytics()

def _flush_analytics():
    if not _analytics_buffer:
        return
    batch = _analytics_buffer[:]
    _analytics_buffer.clear()
    date_str = time.strftime("%Y-%m-%d")
    fpath = _analytics_dir / f"events_{date_str}.jsonl"
    try:
        with open(fpath, 'a', encoding='utf-8') as f:
            for event in batch:
                f.write(json.dumps(event, ensure_ascii=False) + '\n')
    except Exception as e:
        logger.error(f"Analytics write error: {e}")

# Flush analytics every 30 seconds
def _analytics_flush_loop():
    while True:
        time.sleep(30)
        try:
            with _analytics_lock:
                _flush_analytics()
        except Exception:
            pass

_analytics_thread = threading.Thread(target=_analytics_flush_loop, daemon=True)
_analytics_thread.start()

@app.post("/api/v1/analytics")
async def rest_analytics(request: Request):
    """REST endpoint for analytics via sendBeacon."""
    try:
        body = await request.json()
        events = body.get("events", []) if isinstance(body, dict) else []
        _store_analytics(events)
    except Exception:
        pass
    return {"ok": True}


if __name__ == "__main__":
    logger.info(f"Platform: {'Windows' if IS_WINDOWS else 'Linux'}")
    logger.info(f"Engine: {ENGINE_ADAPTER} — {'OK' if ENGINE_ADAPTER.exists() else 'MISSING (battles will simulate without C++ engine)'}")
    logger.info(f"Data: {len(_species_list)} species, {len(_moves)} moves, {len(_abilities)} abilities")
    logger.info("Starting WebSocket server on ws://127.0.0.1:9000/ws ...")
    uvicorn.run(app, host="0.0.0.0", port=9000)
