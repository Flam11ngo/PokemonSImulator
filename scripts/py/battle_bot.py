#!/usr/bin/env python3
"""Auto-match battle bot — stays in queue, auto-plays turns."""
import asyncio, json, websockets, random, sys, os

WS = "ws://127.0.0.1:9000/ws"
BOT_ID = f"bot_{random.randint(1000,9999)}"

# A basic team for the bot
TEAM = {
    "name": "Bot Team",
    "pokemon": [
        {"speciesID": 6, "level": 50, "ability": 66, "nature": 3, "item": 0,
         "moves": [53, 126, 163, 200]},   # Charizard
        {"speciesID": 9, "level": 50, "ability": 67, "nature": 15, "item": 0,
         "moves": [55, 56, 57, 110]},     # Blastoise
        {"speciesID": 3, "level": 50, "ability": 65, "nature": 23, "item": 0,
         "moves": [75, 77, 79, 188]},     # Venusaur
    ]
}

async def send(ws, msg_type, data=None):
    await ws.send(json.dumps({"type": msg_type, "data": data or {}}))

async def recv(ws, timeout=None):
    try:
        raw = await asyncio.wait_for(ws.recv(), timeout=timeout)
        return json.loads(raw)
    except asyncio.TimeoutError:
        return None

async def drain(ws, timeout=0.2):
    """Drain all pending messages."""
    msgs = []
    while True:
        m = await recv(ws, timeout)
        if m is None: break
        msgs.append(m)
    return msgs

async def bot_loop():
    battle_count = 0
    while True:
        try:
            print(f"\n{'='*50}")
            print(f"Bot: {BOT_ID} | Battles: {battle_count}")
            print(f"{'='*50}")

            async with websockets.connect(WS) as ws:
                # Handshake
                await send(ws, "handshake", {"player_id": BOT_ID})
                handshake = await recv(ws, 5)
                if not handshake or handshake.get("type") != "handshake_ok":
                    print("Handshake failed, retrying...")
                    await asyncio.sleep(2)
                    continue
                print("Connected")

                # Join queue
                await send(ws, "join_matchmaking", {
                    "player_id": BOT_ID,
                    "team_json": json.dumps(TEAM)
                })

                # Wait for match
                state = await wait_for_match(ws)
                if not state:
                    continue

                battle_count += 1
                side = state["side"]
                battle_id = state["battle_id"]
                print(f"Battle #{battle_count}: {battle_id} (side {side})")

                # Auto-play turns
                await auto_play(ws, battle_id, side, state["state"])

        except websockets.ConnectionClosed as e:
            print(f"Connection closed: {e}")
            await asyncio.sleep(1)
        except Exception as e:
            print(f"Error: {e}")
            await asyncio.sleep(2)

async def wait_for_match(ws):
    """Wait for matched message, handling intermediate messages."""
    while True:
        msg = await recv(ws, 30)
        if msg is None:
            print("Matchmaking timeout, retrying...")
            return None
        t = msg["type"]
        if t == "matched":
            return msg["data"]
        elif t == "match_found":
            print("  Match found, engine starting...")
        elif t == "matchmaking_status":
            d = msg["data"]
            print(f"  Queue: {d.get('status')} (pool: {d.get('pool_size', '?' )})")
        elif t == "match_cancelled":
            print(f"  Match cancelled: {msg['data'].get('message')}")
            return None

async def auto_play(ws, battle_id, side, initial_state):
    """Auto-play turns until battle ends."""
    active = True
    while active:
        msgs = await drain(ws, timeout=1.0)
        for msg in msgs:
            t = msg["type"]
            d = msg.get("data", {})

            if t == "turn_processed":
                status = d.get("status", "active")
                if status == "completed":
                    print("  Battle ended!")
                    active = False
                else:
                    # Submit action for next turn
                    await submit_action(ws, battle_id, side, d.get("state"))
            elif t == "opponent_disconnected":
                print(f"  Opponent left: {d.get('message')}")
                active = False
            elif t == "force_switch_done":
                pass  # Just wait for next turn

        if active:
            await asyncio.sleep(0.5)

async def submit_action(ws, battle_id, side, state):
    """Choose and submit an action."""
    if not state:
        action = {"side": side, "type": "pass"}
    else:
        # Get our active Pokemon's moves
        sides = state.get("battle", state).get("sides", [])
        our_side = sides[0] if side == "a" else sides[1] if len(sides) > 1 else None
        if not our_side:
            action = {"side": side, "type": "pass"}
        else:
            active_idx = our_side.get("active", 0)
            pokemons = our_side.get("pokemons", [])
            if active_idx >= len(pokemons):
                action = {"side": side, "type": "pass"}
            else:
                active_pkm = pokemons[active_idx]
                moves = active_pkm.get("moves", [])
                # Always use first move with PP > 0
                usable = [m for m in moves if m.get("pp", 0) > 0]
                if usable:
                    move_idx = 0  # always first usable move
                    action = {"side": side, "type": "attack", "move_index": move_idx}
                    print(f"  Attacking with move {usable[0]['id']} (idx {move_idx})")
                else:
                    action = {"side": side, "type": "pass"}
                    print("  No usable moves, passing")

    await send(ws, "submit_action", {"battle_id": battle_id, "action": action})

if __name__ == "__main__":
    print(f"Battle Bot starting: {BOT_ID}")
    print("Stays in queue indefinitely. Ctrl+C to stop.")
    try:
        asyncio.run(bot_loop())
    except KeyboardInterrupt:
        print("\nBot stopped.")
