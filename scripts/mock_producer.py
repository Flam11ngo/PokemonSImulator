"""
Mock Kafka producer — generates realistic battle + UI data at high speed.
Multiple concurrent battles, proper damage/heal/ability/switch turn sequences.
Usage:  python3 mock_producer.py [battles_per_sec]
"""
import json, random, sys, time, threading
from datetime import datetime
from kafka import KafkaProducer

BROKER = "100.107.105.99:9092"
BATTLES_PER_SEC = float(sys.argv[1]) if len(sys.argv) > 1 else 2

# ── Game data ──
SPECIES_DB = {
    6:  {"name":"Charizard",   "hp":297,"atk":183,"def":172,"spa":239,"spd":185,"spe":236,"types":["Fire","Flying"]},
    9:  {"name":"Blastoise",   "hp":299,"atk":181,"def":236,"spa":185,"spd":246,"spe":172,"types":["Water"]},
    3:  {"name":"Venusaur",    "hp":301,"atk":180,"def":185,"spa":236,"spd":236,"spe":176,"types":["Grass","Poison"]},
    25: {"name":"Pikachu",     "hp":211,"atk":167,"def":116,"spa":167,"spd":156,"spe":216,"types":["Electric"]},
    149:{"name":"Dragonite",   "hp":323,"atk":296,"def":226,"spa":236,"spd":236,"spe":176,"types":["Dragon","Flying"]},
    445:{"name":"Garchomp",    "hp":357,"atk":296,"def":226,"spa":176,"spd":185,"spe":239,"types":["Dragon","Ground"]},
    448:{"name":"Lucario",     "hp":281,"atk":246,"def":176,"spa":257,"spd":176,"spe":216,"types":["Fighting","Steel"]},
    658:{"name":"Greninja",    "hp":291,"atk":226,"def":170,"spa":240,"spd":174,"spe":284,"types":["Water","Dark"]},
    778:{"name":"Mimikyu",     "hp":261,"atk":216,"def":176,"spa":167,"spd":246,"spe":230,"types":["Ghost","Fairy"]},
    823:{"name":"Corviknight", "hp":333,"atk":203,"def":246,"spa":157,"spd":185,"spe":170,"types":["Flying","Steel"]},
    887:{"name":"Dragapult",   "hp":317,"atk":248,"def":176,"spa":236,"spd":176,"spe":298,"types":["Dragon","Ghost"]},
    901:{"name":"Ursaluna",    "hp":393,"atk":296,"def":246,"spa":113,"spd":176,"spe":136,"types":["Normal","Ground"]},
    983:{"name":"Kingambit",   "hp":341,"atk":308,"def":248,"spa":140,"spd":185,"spe":136,"types":["Dark","Steel"]},
}
MOVE_DB = [
    {"id":85,"name":"Thunderbolt","type":"Electric","power":90,"category":"Special"},
    {"id":89,"name":"Earthquake","type":"Ground","power":100,"category":"Physical"},
    {"id":94,"name":"Flamethrower","type":"Fire","power":90,"category":"Special"},
    {"id":57,"name":"Surf","type":"Water","power":90,"category":"Special"},
    {"id":247,"name":"Shadow Ball","type":"Ghost","power":80,"category":"Special"},
    {"id":369,"name":"U-turn","type":"Bug","power":70,"category":"Physical"},
    {"id":370,"name":"Close Combat","type":"Fighting","power":120,"category":"Physical"},
    {"id":59,"name":"Ice Beam","type":"Ice","power":90,"category":"Special"},
    {"id":394,"name":"Dragon Pulse","type":"Dragon","power":85,"category":"Special"},
    {"id":14,"name":"Swords Dance","type":"Normal","power":0,"category":"Status"},
    {"id":182,"name":"Protect","type":"Normal","power":0,"category":"Status"},
    {"id":188,"name":"Stealth Rock","type":"Rock","power":0,"category":"Status"},
    {"id":242,"name":"Crunch","type":"Dark","power":80,"category":"Physical"},
    {"id":444,"name":"Stone Edge","type":"Rock","power":100,"category":"Physical"},
    {"id":416,"name":"Giga Impact","type":"Normal","power":150,"category":"Physical"},
    {"id":349,"name":"Dragon Dance","type":"Dragon","power":0,"category":"Status"},
    {"id":403,"name":"Air Slash","type":"Flying","power":75,"category":"Special"},
    {"id":280,"name":"Brick Break","type":"Fighting","power":75,"category":"Physical"},
    {"id":201,"name":"Sandstorm","type":"Rock","power":0,"category":"Status"},
]
ITEMS = [211,234,275,287,188,297,184,185,219,220,221,232,236,240]
ABILITIES = [
    {"id":9,"name":"Static"},{"id":3,"name":"Speed Boost"},{"id":26,"name":"Levitate"},
    {"id":66,"name":"Blaze"},{"id":168,"name":"Mold Breaker"},{"id":45,"name":"Shed Skin"},
    {"id":146,"name":"Sand Veil"},{"id":67,"name":"Intimidate"},{"id":10,"name":"Volt Absorb"},
    {"id":104,"name":"Technician"},{"id":145,"name":"Big Pecks"},{"id":186,"name":"Regenerator"},
]
NATURES = [3,10,15,5,8,13,20,23,0]
PLAYERS = ["Ash","Serena","Leon","Cynthia","Red","Blue"]
PAGES = ["/","/matchmaking","/teams","/stats","/data"]
CLICKS = ["btn_confirm","btn_switch","btn_move_select","btn_join_match","btn_join_bot",
          "btn_save_team","nav_matchmaking","nav_teams","nav_stats","nav_data"]
TYPE_CHART = {
    ("Fire","Grass"):2,("Water","Fire"):2,("Electric","Water"):2,("Grass","Water"):2,
    ("Ice","Dragon"):2,("Dragon","Dragon"):2,("Fighting","Normal"):2,("Ground","Electric"):2,
    ("Ghost","Psychic"):2,("Dark","Ghost"):2,("Fairy","Dragon"):2,("Steel","Fairy"):2,
    ("Rock","Flying"):2,("Bug","Psychic"):2,("Poison","Fairy"):2,("Flying","Fighting"):2,
}

producer = KafkaProducer(bootstrap_servers=BROKER, api_version=(3,9), acks=1,
    value_serializer=lambda v: json.dumps(v,ensure_ascii=False).encode("utf-8"))
_lock = threading.Lock()


def ts(): return datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ")
def rid(): return f"{random.randint(1000,9999):04d}"

def send(topic, key, value):
    value["timestamp"] = ts()
    with _lock:
        producer.send(topic, key=key.encode() if key else None, value=value)

def dmg_calc(move, atk_sp, def_sp, atk_stat, def_stat):
    """Approximate damage formula (simplified Smogon)."""
    if move["power"] == 0: return 0
    cat = "Physical" if move["category"] == "Physical" else "Special"
    atk = atk_sp if cat == "Physical" else (atk_sp if cat == "Physical" else atk_sp)
    # rough: level 50, 252 EV
    A = int(((2*50/5+2)*move["power"]*atk/def_sp)/50 + 2)
    # stab, type effectiveness, random
    stab = 1.5 if random.random() < 0.4 else 1.0
    eff = random.choice([0.5,1.0,2.0])
    r = random.uniform(0.85,1.0)
    return max(1,int(A*stab*eff*r))

class Battle:
    def __init__(self):
        self.bid = "mock_"+rid()
        self.turn = 0
        self.team_size = random.randint(3,6)
        self._make_teams()
        self._send_init()

    def _make_team(self):
        ids = random.sample(list(SPECIES_DB), self.team_size)
        return [{"speciesID":sid,"moves":[m["id"] for m in random.sample(MOVE_DB,4)],
                 "item":random.choice(ITEMS),"ability":random.choice(ABILITIES)["id"],
                 "nature":random.choice(NATURES),"level":50,"_name":SPECIES_DB[sid]["name"],
                 "_stats":SPECIES_DB[sid]} for sid in ids]

    def _make_teams(self):
        self.side_a = self._make_team()
        self.side_b = self._make_team()
        self.active_a = 0; self.active_b = 0
        self.hp_a = {i:p["_stats"]["hp"] for i,p in enumerate(self.side_a)}
        self.hp_b = {i:p["_stats"]["hp"] for i,p in enumerate(self.side_b)}
        self.alive_a = set(range(self.team_size))
        self.alive_b = set(range(self.team_size))

    def _send_init(self):
        send("battle.logs",self.bid,{"event":"battle_init","data":{
            "battle_id":self.bid,"opponent_type":"bot",
            "side_a":[{k:p[k] for k in ["speciesID","moves","item","ability","nature","level"]} for p in self.side_a],
            "side_b":[{k:p[k] for k in ["speciesID","moves","item","ability","nature","level"]} for p in self.side_b],
        }})

    def step(self):
        """One turn — both sides act, damage/heal/faint/switches possible."""
        self.turn += 1
        a_act = random.choices(["attack","attack","attack","status","switch","pass"],weights=[5,5,5,2,1,1])[0]
        b_act = random.choices(["attack","attack","attack","status","switch","pass"],weights=[5,5,5,2,1,1])[0]

        send("battle.logs",self.bid,{"event":"turn_executed","data":{
            "battle_id":self.bid,"turn":self.turn,
            "side_a":{"type":a_act,"move_id":self._move_for(a_act,"a")["id"],"move_name":self._move_for(a_act,"a")["name"]} if a_act in ("attack","status") else {"type":a_act},
            "side_b":{"type":b_act,"move_id":self._move_for(b_act,"b")["id"],"move_name":self._move_for(b_act,"b")["name"]} if b_act in ("attack","status") else {"type":b_act},
        }})

        # Resolve damage
        for actor_side, target_side in [("a","b"),("b","a")]:
            act = a_act if actor_side == "a" else b_act
            if act not in ("attack","status"): continue
            mv = self._move_for(act, actor_side)
            if mv["power"] == 0:
                send("battle.logs",self.bid,{"event":"turn_heal","data":{
                    "battle_id":self.bid,"turn":self.turn,"target_side":actor_side,
                    "target_species":self._active(actor_side)["speciesID"],
                    "heal":random.randint(20,80)}})
                continue

            tgt = self._active(target_side)
            dmg = dmg_calc(mv, self._active(actor_side)["_stats"]["atk"], tgt["_stats"]["def"],
                           self._active(actor_side)["_stats"]["atk"], tgt["_stats"]["def"])
            idx = self.active_b if target_side == "b" else self.active_a
            hp_pool = self.hp_b if target_side == "b" else self.hp_a
            hp_pool[idx] = max(0, hp_pool[idx] - dmg)
            fainted = hp_pool[idx] <= 0

            send("battle.logs",self.bid,{"event":"turn_damage","data":{
                "battle_id":self.bid,"turn":self.turn,"target_side":target_side,
                "target_species":tgt["speciesID"],"move":mv["name"],
                "damage":dmg,"fainted":fainted}})

            if fainted:
                alive = self.alive_b if target_side == "b" else self.alive_a
                alive.discard(idx)
                send("battle.logs",self.bid,{"event":"turn_faint","data":{
                    "battle_id":self.bid,"turn":self.turn,
                    "species":tgt["speciesID"],"side":target_side}})
                # Force switch
                if alive:
                    new_idx = random.choice(list(alive))
                    if target_side == "b": self.active_b = new_idx
                    else: self.active_a = new_idx
                    send("battle.logs",self.bid,{"event":"turn_switch","data":{
                        "battle_id":self.bid,"turn":self.turn,
                        "species":(self.side_b[new_idx] if target_side=="b" else self.side_a[new_idx])["speciesID"],
                        "side":target_side,"reason":"faint_replace"}})
                    send("battle.logs",self.bid,{"event":"turn_ability","data":{
                        "battle_id":self.bid,"turn":self.turn,
                        "species":(self.side_b[new_idx] if target_side=="b" else self.side_a[new_idx])["speciesID"],
                        "side":target_side,"ability":random.choice(ABILITIES)["name"]}})

    def _active(self, side):
        idx = self.active_a if side == "a" else self.active_b
        return self.side_a[idx] if side == "a" else self.side_b[idx]

    def _move_for(self, act, side):
        p = self._active(side)
        moves = [m for m in MOVE_DB if m["id"] in p.get("moves",[])]
        return random.choice(moves) if moves else random.choice(MOVE_DB)

    def is_over(self):
        return not self.alive_a or not self.alive_b or self.turn >= 15

    def finish(self):
        a_left = len(self.alive_a); b_left = len(self.alive_b)
        if a_left > 0 and b_left == 0: winner = "Player"; result = "completed"
        elif b_left > 0 and a_left == 0: winner = "Opponent"; result = "completed"
        else: winner = None; result = "draw"
        send("battle.logs",self.bid,{"event":"battle_result","data":{
            "battle_id":self.bid,"result":result,"winner":winner,
            "turns":self.turn,"own_remaining":a_left,"opp_remaining":b_left}})

def gen_ui():
    player = random.choice(PLAYERS)
    evt = random.choice(["page_view","ui_click","player_state","matchmaking_join","team_save"])
    if evt == "page_view":
        send("player.ui.events",player,{"event":"page_view","player_id":player,"data":{"page":random.choice(PAGES)}})
    elif evt == "ui_click":
        send("player.ui.events",player,{"event":"ui_click","player_id":player,"data":{"element":random.choice(CLICKS)}})
    elif evt == "player_state":
        send("player.ui.events",player,{"event":"player_state","player_id":player,"data":{"from":"idle","to":random.choice(["matching","battling","teambuilding"])}})
    elif evt == "matchmaking_join":
        send("player.ui.events",player,{"event":"matchmaking_join","player_id":player,"data":{"opponent_type":random.choice(["human","bot"])}})
    elif evt == "team_save":
        team = [{"speciesID":random.choice(list(SPECIES_DB)),"moves":[m["id"] for m in random.sample(MOVE_DB,4)],"item":random.choice(ITEMS),"ability":random.choice(ABILITIES)["id"],"nature":random.choice(NATURES)} for _ in range(random.randint(3,6))]
        send("player.ui.events",player,{"event":"team_save","player_id":player,"data":{"team_name":random.choice(["晴天队","雨天队","沙暴队","龙队","平衡队","空间队"]),"pokemon_count":len(team),"pokemon":team}})

def main():
    print(f"Mock producer: {BATTLES_PER_SEC} battles/sec, broker={BROKER}")
    battles = []
    tick = 0
    while True:
        tick += 1
        # Start new battles
        for _ in range(max(1, int(BATTLES_PER_SEC))):
            battles.append(Battle())
        # Step all active battles
        for b in battles[:]:
            b.step()
            if b.is_over():
                b.finish()
                battles.remove(b)
        # UI events
        for _ in range(random.randint(2,5)):
            gen_ui()
        producer.flush()
        time.sleep(1)

if __name__ == "__main__":
    main()
