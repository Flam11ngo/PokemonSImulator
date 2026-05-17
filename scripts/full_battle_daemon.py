#!/usr/bin/env python3
"""
完整 3v3 宝可梦对战 -- Daemon / Cache 模式演示。

展示机制：
  特性: Intimidate, Sand Stream, Levitate, Serene Grace
  道具: Leftovers, Sitrus Berry, Life Orb, Heavy-Duty Boots
  天气: Sandstorm (via Sand Stream)
  变化技能: Dragon Dance, Swords Dance, Nasty Plot, Stealth Rock, Will-O-Wisp, Charm
  技能附加效果: Waterfall (畏缩), Ice Fang (冰冻+畏缩), Air Slash (畏缩)
  场地: Stealth Rock

用法:
  python3 scripts/full_battle_daemon.py cache   # 推荐：自动批处理
  python3 scripts/full_battle_daemon.py daemon  # 真实 daemon 模式
"""

import json, os, sys, subprocess, time

CACHE_IN  = "cache/input"
CACHE_OUT = "cache/output"
SIM       = "./build/PokemonSimulator"

# ── 数据索引 (来自 data/*.json) ──────────────────────────────────
SP = {"Gyarados":130,"Scizor":212,"Tyranitar":248,"Garchomp":445,"Rotom":479,"Togekiss":468}
AB = {"Intimidate":22,"RoughSkin":24,"Levitate":26,"SereneGrace":32,"SandStream":45,"Technician":101}
IT = {"LifeOrb":247,"Leftovers":211,"RockyHelmet":583,"SitrusBerry":135,"HeavyDutyBoots":1178}
MV = {
    "Waterfall":127,"IceFang":423,"DragonDance":349,"Earthquake":89,
    "SwordsDance":14,"BulletPunch":418,"Uturn":369,"Roost":355,
    "StoneEdge":444,"Crunch":242,"StealthRock":446,
    "DragonClaw":337,"FireFang":424,
    "WillOWisp":261,"HydroPump":56,"VoltSwitch":521,"ThunderWave":86,
    "NastyPlot":417,"AirSlash":403,"Charm":204,"Tackle":33,
}

IV = {"hp":31,"attack":31,"defense":31,"specialAttack":31,"specialDefense":31,"speed":31}
EV_ATK  = {"hp":4,"attack":252,"defense":0,"specialAttack":0,"specialDefense":0,"speed":252}
EV_SPA  = {"hp":4,"attack":0,"defense":0,"specialAttack":252,"specialDefense":0,"speed":252}
EV_BULK = {"hp":252,"attack":0,"defense":128,"specialAttack":0,"specialDefense":128,"speed":0}
EV_LEAD = {"hp":80,"attack":128,"defense":48,"specialAttack":0,"specialDefense":48,"speed":252}
EV_TANK = {"hp":252,"attack":0,"defense":252,"specialAttack":0,"specialDefense":4,"speed":0}

def pokemon(species, ability, item, moves, nature=3, evs=None):
    return {"speciesID":SP[species],"level":50,"nature":nature,
            "ability":AB[ability],"item":IT[item],"ivs":IV,
            "evs":evs or EV_ATK,"moves":[MV[m] for m in moves]}

INIT = {
    "side_a": {"name": "Side A", "pokemon": [
        pokemon("Gyarados","Intimidate","LifeOrb",
                ["Waterfall","IceFang","DragonDance","Earthquake"],evs=EV_LEAD),
        pokemon("Scizor","Technician","Leftovers",
                ["SwordsDance","BulletPunch","Uturn","Roost"],evs=EV_BULK),
        pokemon("Tyranitar","SandStream","Leftovers",
                ["StoneEdge","Crunch","StealthRock","Earthquake"],evs=EV_BULK),
    ]},
    "side_b": {"name": "Side B", "pokemon": [
        pokemon("Garchomp","RoughSkin","RockyHelmet",
                ["SwordsDance","Earthquake","DragonClaw","FireFang"],evs=EV_LEAD),
        pokemon("Rotom","Levitate","SitrusBerry",
                ["WillOWisp","HydroPump","VoltSwitch","ThunderWave"],evs=EV_TANK),
        pokemon("Togekiss","SereneGrace","HeavyDutyBoots",
                ["NastyPlot","AirSlash","ThunderWave","Roost"],evs=EV_SPA),
    ]},
    "seed": 42,
}

def act(side, move=None, switch_idx=None):
    if switch_idx is not None: return {"side":side,"type":"switch","switch_index":switch_idx}
    return {"side":side,"type":"attack","move_name":move}

# ═══════════════════════════════════════════════════════════════
# 回合设计 (使用 move_name 避免换人后技能错位)
# ═══════════════════════════════════════════════════════════════
# T1: 双方变化技能 — Dragon Dance (+1Atk+1Spe) / Swords Dance (+2Atk)
# T2: 攻击交换 — Waterfall (接触,RoughSkin反弹) / Earthquake
# T3: A换Tyranitar (SandStream→沙暴) / B DragonClaw
# T4: A StealthRock (场地:隐形岩) / B Earthquake → Tyranitar濒死→暴鲤龙返场
# T5: A IceFang (非本系,不打残Rotom) / B换Rotom (Levitate)
# T6: A IceFang / B WillOWisp (灼伤)
# T7: A换Scizor / B ThunderWave (麻痹Scizor)
# T8: A SwordsDance / B换Togekiss
# T9: A Roost (回复) / B AirSlash (SereneGrace畏缩翻倍+伤害)
TURNS = [
    (act("a", move="Dragon Dance"),  act("b", move="Swords Dance")),   # T1
    (act("a", move="Waterfall"),     act("b", move="Earthquake")),     # T2
    (act("a", switch_idx=2),         act("b", move="Dragon Claw")),    # T3
    (act("a", move="Stealth Rock"),  act("b", move="Earthquake")),     # T4
    (act("a", move="Ice Fang"),      act("b", switch_idx=1)),          # T5
    (act("a", move="Ice Fang"),      act("b", move="Will O Wisp")),    # T6
    (act("a", switch_idx=1),         act("b", move="Thunder Wave")),   # T7
    (act("a", move="Swords Dance"),  act("b", switch_idx=2)),          # T8
    (act("a", move="Roost"),         act("b", move="Air Slash")),      # T9
]

# ── 工具函数 ────────────────────────────────────────────────────
def clear_cache():
    for d in [CACHE_IN, CACHE_OUT]:
        os.makedirs(d, exist_ok=True)
        for f in os.listdir(d):
            p = os.path.join(d, f)
            if os.path.isfile(p): os.remove(p)

def write_all_inputs():
    clear_cache()
    with open(f"{CACHE_IN}/init_request.json", "w") as f: json.dump(INIT, f, indent=2)
    for i, (a, b) in enumerate(TURNS, 1):
        with open(f"{CACHE_IN}/1_input_{i}.json", "w") as f: json.dump(a, f)
        with open(f"{CACHE_IN}/2_input_{i}.json", "w") as f: json.dump(b, f)
    print(f"[OK] init + {len(TURNS)} turn pairs written")

def print_outputs():
    files = sorted(
        [f for f in os.listdir(CACHE_OUT) if f.startswith("output_") and f.endswith(".json")],
        key=lambda x: int(x.split("_")[1].split(".")[0]))
    for fname in files:
        with open(f"{CACHE_OUT}/{fname}") as f: data = json.load(f)
        turn = data.get("turn", "?")
        descs = data.get("descriptions", data.get("description", []))
        battle = data.get("battle", data.get("battle_all_info", {}).get("battle", {}))
        print(f"\n{'='*60}\n  TURN {turn}\n{'='*60}")
        if descs:
            for d in descs: print(f"  • {d}")
        else:
            print("  (no events)")
        for side in battle.get("sides", []):
            for p in side.get("pokemons", []):
                hp, mhp = p.get("hp","?"), p.get("maxHp","?")
                stages = p.get("statStages", [])
                st = ""
                if any(s != 0 for s in stages[:5]):
                    nm = ["Atk","Def","SpA","SpD","Spe"]
                    st = " [" + ", ".join(f"{n}{s:+d}" for n, s in zip(nm, stages[:5]) if s != 0) + "]"
                dead = " [FAINTED]" if p.get("fainted") else ""
                print(f"    {side['name']} slot{p['slot']} HP={hp}/{mhp}{st}{dead}")
        w = battle.get("weather", {}); fld = battle.get("field", {})
        if w.get("type", 0) != 0: print(f"  Weather: type={w['type']} dur={w.get('duration','?')}")
        if fld.get("type", 0) != 0: print(f"  Field: type={fld['type']} dur={fld.get('duration','?')}")

# ── 运行模式 ────────────────────────────────────────────────────
def run_cache():
    write_all_inputs()
    r = subprocess.run([SIM, "--run-cache-input"], capture_output=True, text=True, timeout=60)
    if r.returncode != 0:
        # 检查是否有输出文件（即使提前结束）
        if os.path.exists(f"{CACHE_OUT}/output_0.json"):
            print_outputs()
        else:
            print(f"[FAIL] {r.stderr}")
    else:
        print_outputs()

def run_daemon():
    """真实 daemon 模式：启动守护进程，逐步写入回合文件。"""
    clear_cache()
    with open(f"{CACHE_IN}/init_request.json", "w") as f: json.dump(INIT, f, indent=2)
    print("[DAEMON] Starting simulator...")

    proc = subprocess.Popen([SIM, "--daemon"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    time.sleep(1.5)

    for i, (a, b) in enumerate(TURNS, 1):
        # 写入本回合双方动作
        with open(f"{CACHE_IN}/1_input_{i}.json", "w") as f: json.dump(a, f)
        with open(f"{CACHE_IN}/2_input_{i}.json", "w") as f: json.dump(b, f)

        out_file = f"{CACHE_OUT}/output_{i}.json"
        for _ in range(60):  # 最多等 30 秒
            if os.path.exists(out_file): break
            time.sleep(0.5)

        if os.path.exists(out_file):
            print(f"\n── Turn {i} ──")
            with open(out_file) as f:
                data = json.load(f)
            for d in data.get("descriptions", data.get("description", [])):
                print(f"  {d}")

        go_file = f"{CACHE_OUT}/game_over.json"
        if os.path.exists(go_file):
            with open(go_file) as f: go = json.load(f)
            print(f"\n[GAME OVER] Winner: {go.get('winner','?')}, Final turn: {go.get('final_turn','?')}")
            break

    proc.terminate(); proc.wait(timeout=5)
    print(f"\n[DAEMON] Done. Output files in {CACHE_OUT}/")

if __name__ == "__main__":
    mode = sys.argv[1] if len(sys.argv) > 1 else "cache"
    if mode == "daemon": run_daemon()
    else: run_cache()
