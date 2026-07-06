#!/usr/bin/env python3
"""
演示对战: 场地 / 天气 / 道具 / 技能附加效果 / 正面Buff
通过 WebSocket 连接本地服务器进行全流程演示
"""
import asyncio, json, websockets, time

WS = "ws://localhost:8000/ws"

async def send(ws, msg_type, data=None):
    await ws.send(json.dumps({"type": msg_type, "data": data or {}}))

async def recv(ws):
    return json.loads(await ws.recv())

def print_state(state, label=""):
    sides = state.get("battle", state).get("sides", [])
    weather = state.get("battle", state).get("weather", {})
    field = state.get("battle", state).get("field", {})
    events = state.get("events", [])

    print(f"\n{'='*60}")
    print(f"  {label}")
    print(f"  天气: {weather.get('type','?')}  场地: {field.get('type','?')}")
    for sd in sides:
        eff = sd.get("sideEffects", {})
        print(f"  {sd['name']}:")
        for p in sd.get("pokemons", []):
            if p.get("fainted"): continue
            stats = p.get("statStages", [0]*7)
            status = [s.get("id") for s in p.get("inBattleStatus", []) if s.get("id") != 0]
            item = p.get("itemId", 0)
            print(f"    {p.get('_speciesName','?')}: HP={p['hp']}/{p['maxHp']} "
                  f"Atk{stats[0]:+d} Def{stats[1]:+d} SpA{stats[2]:+d} SpD{stats[3]:+d} Spe{stats[4]:+d} "
                  f"状态={status} 道具={item}")
        # Side effects
        se = []
        if eff.get("reflect"): se.append("Reflect")
        if eff.get("lightScreen"): se.append("LightScreen")
        if eff.get("tailwind"): se.append("Tailwind")
        if eff.get("spikes"): se.append(f"Spikes({eff['spikes']})")
        if eff.get("stealthRock"): se.append("StealthRock")
        if se: print(f"    场地效果: {', '.join(se)}")
    print(f"  事件: {len(events)}条")
    for e in events[-5:]:
        print(f"    [{e.get('event_type','?')}] {e.get('description','')[:60]}")


async def main():
    # ============================================================
    # 队伍: Side A 负责展示 Buff / 场地 / 天气
    #       Side B 负责展示 附加效果 / 道具
    # ============================================================

    team_a = {
        "name": "演示方A",
        "pokemon": [
            {   # Zapdos @ Leftovers — 天气 + 场地
                "speciesID": 145, "level": 50, "ability": 10, "nature": 10, "item": 234,
                "moves": [311, 240, 604, 113]  # Weather Ball, Rain Dance, Electric Terrain, Light Screen
            },
            {   # Scizor @ Life Orb — Buff
                "speciesID": 212, "level": 50, "ability": 101, "nature": 3, "item": 270,
                "moves": [14, 332, 104, 116]  # Swords Dance, Aerial Ace, Agility, Focus Energy
            },
            {   # Ferrothorn — 场地效果
                "speciesID": 598, "level": 50, "ability": 160, "nature": 7, "item": 275,
                "moves": [388, 390, 115, 73]  # Stealth Rock, Spikes, Reflect, Leech Seed
            },
        ]
    }

    team_b = {
        "name": "演示方B",
        "pokemon": [
            {   # Venusaur @ Black Sludge — 附加效果
                "speciesID": 3, "level": 50, "ability": 65, "nature": 15, "item": 281,
                "moves": [75, 77, 79, 188]  # Razor Leaf, Poison Powder, Sleep Powder, Sludge Bomb
            },
            {   # Gengar @ Focus Sash — 附加效果
                "speciesID": 94, "level": 50, "ability": 26, "nature": 10, "item": 275,
                "moves": [94, 247, 261, 109]  # Psychic, Shadow Ball, Will-O-Wisp, Confuse Ray
            },
        ]
    }

    print("=" * 60)
    print("  PokemonSimulator 功能演示对战")
    print("  演示: 场地 → 天气 → 道具 → 附加效果 → Buff")
    print("=" * 60)

    # Connect
    ws = await websockets.connect(WS)
    await send(ws, "handshake", {"player_id": "demo"})
    await recv(ws)

    # Create battle
    t0 = time.time()
    await send(ws, "create_battle", {
        "team_a_json": json.dumps(team_a),
        "team_b_json": json.dumps(team_b),
        "seed": 42
    })
    r = await recv(ws)
    battle = r["data"]["battle"]
    bid = battle["id"]
    state = r["data"]["state"]
    print_state(state, f"初始上场 (Battle #{bid})")

    def act(side, move_idx):
        return {"side": side, "type": "attack", "move_index": move_idx}

    # ============================================================
    # Turn 1: 天气演示 — Rain Dance
    # ============================================================
    print("\n" + "=" * 60)
    print("  🌧️  Turn 1: 天气演示 — Zapdos 使用 Rain Dance")
    print("=" * 60)
    await send(ws, "process_turn", {"battle_id": bid, "actions": [act("a", 1), act("b", 0)]})
    r = await recv(ws)
    print_state(r["data"]["state"], "Turn 1 结果")

    # ============================================================
    # Turn 2: 场地演示 — Electric Terrain
    # ============================================================
    print("\n" + "=" * 60)
    print("  ⚡ Turn 2: 场地演示 — Zapdos 使用 Electric Terrain")
    print("=" * 60)
    await send(ws, "process_turn", {"battle_id": bid, "actions": [act("a", 2), act("b", 1)]})
    r = await recv(ws)
    print_state(r["data"]["state"], "Turn 2 结果")

    # ============================================================
    # Turn 3: 道具演示 — Leftovers 回复 + 附加效果 Poison Powder
    # ============================================================
    print("\n" + "=" * 60)
    print("  🍎 Turn 3: 道具演示 — Leftovers恢复 + Poison Powder中毒")
    print("=" * 60)
    await send(ws, "process_turn", {"battle_id": bid, "actions": [act("a", 0), act("b", 1)]})
    r = await recv(ws)
    print_state(r["data"]["state"], "Turn 3 结果")

    # ============================================================
    # Turn 4: Buff演示 — Swords Dance + Light Screen (换人)
    # ============================================================
    print("\n" + "=" * 60)
    print("  ⚔️  Turn 4: 换人 — Scizor 上场, 使用 Swords Dance (+2 Atk)")
    print("=" * 60)
    await send(ws, "process_turn", {
        "battle_id": bid,
        "actions": [
            {"side": "a", "type": "switch", "switch_index": 1},  # Switch to Scizor
            act("b", 3)  # Sludge Bomb (poison chance)
        ]
    })
    r = await recv(ws)
    print_state(r["data"]["state"], "Turn 4 结果")

    # ============================================================
    # Turn 5: Buff演示 — Agility (+2 Spe) + Will-O-Wisp (烧伤)
    # ============================================================
    print("\n" + "=" * 60)
    print("  💨 Turn 5: Buff演示 — Scizor Agility(+2Spe) vs Gengar Will-O-Wisp(烧伤)")
    print("=" * 60)
    await send(ws, "process_turn", {
        "battle_id": bid,
        "actions": [
            act("a", 2),  # Agility
            {"side": "b", "type": "switch", "switch_index": 1},  # Switch to Gengar
        ]
    })
    r = await recv(ws)
    print_state(r["data"]["state"], "Turn 5 结果")

    # ============================================================
    # Turn 6: Light Screen + Confuse Ray + 附加效果 Sleep Powder
    # ============================================================
    print("\n" + "=" * 60)
    print("  🛡️  Turn 6: Light Screen + Will-O-Wisp烧伤 + Confuse Ray混乱")
    print("=" * 60)
    await send(ws, "process_turn", {"battle_id": bid, "actions": [act("a", 3), act("b", 2)]})
    r = await recv(ws)
    print_state(r["data"]["state"], "Turn 6 结果")

    # ============================================================
    # Turn 7: 换回 Ferrothorn — Stealth Rock + Spikes + Leech Seed
    # ============================================================
    print("\n" + "=" * 60)
    print("  🪨 Turn 7: 场地效果 — Stealth Rock + Spikes + Leech Seed")
    print("=" * 60)
    await send(ws, "process_turn", {
        "battle_id": bid,
        "actions": [
            {"side": "a", "type": "switch", "switch_index": 2},  # Switch to Ferrothorn
            act("b", 1),  # Shadow Ball
        ]
    })
    r = await recv(ws)
    print_state(r["data"]["state"], "Turn 7 结果")

    print("\n" + "=" * 60)
    print(f"  ✅ 演示完成! ({time.time()-t0:.1f}s)")
    print("  已展示: 天气(雨天) | 场地(电场) | 道具(Leftovers) |")
    print("         附加效果(中毒/烧伤/混乱) | Buff(SwordsDance/Agility/LightScreen)")
    print("         场地陷阱(StealthRock/Spikes/LeechSeed)")
    print("=" * 60)

    await ws.close()

if __name__ == "__main__":
    asyncio.run(main())
