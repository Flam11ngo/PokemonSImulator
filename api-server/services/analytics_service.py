"""
Unified analytics & battle log pipeline.
- Writes to local JSONL files (always)
- Forwards to Kafka (optional, best-effort)
"""
from __future__ import annotations
import json
import logging
import threading
import time
from datetime import datetime, timezone
from pathlib import Path
try:
    from services.kafka_producer import send_ui_event, send_battle_log, start as kafka_start
except Exception:
    send_ui_event = lambda e: None
    send_battle_log = lambda e: None
    kafka_start = lambda: None

logger = logging.getLogger("analytics-service")


class AnalyticsService:
    """Singleton: collect, buffer, flush events to file + Kafka."""

    def __init__(self, log_dir: Path):
        self.log_dir = log_dir
        self.log_dir.mkdir(parents=True, exist_ok=True)
        self._lock = threading.Lock()
        self._buffer = []
        self._flush_timer = None
        kafka_start()
        self._start_flush_timer()
        self._start_battle_importer()

    def _start_battle_importer(self):
        """Periodically import battle state snapshots into SQLite for stats queries."""
        try:
            from battle_importer import import_new_battles
        except ImportError:
            return
        def _loop():
            time.sleep(10)  # wait for engine to start
            while True:
                try:
                    import_new_battles()
                except Exception:
                    pass
                time.sleep(30)  # every 30 seconds
        t = threading.Thread(target=_loop, daemon=True)
        t.start()
        logger.info("Battle importer started (30s interval)")

    # ── Public API ──────────────────────────────

    def track(self, event: str, data: dict, battle_id: str = "", player_id: str = ""):
        """Record a UI analytics event."""
        entry = self._make_entry(event, data, player_id)
        self._enqueue(entry)
        # non-blocking Kafka
        send_ui_event(entry)

    def log_battle_init(self, battle_id: str, state: dict):
        """Record full team data at battle start — format matches data_daemon."""
        sides = state.get("battle", state).get("sides", [])
        side_a = sides[0] if len(sides) > 0 else {}
        side_b = sides[1] if len(sides) > 1 else {}
        p1 = side_a.get("name", "Player1")
        p2 = side_b.get("name", "Player2")

        # Extract pokemon data in daemon format: {speciesID, moves[], item, ability, nature, level}
        def extract_team(side):
            mons = []
            for pkm in side.get("pokemons", []) or []:
                # Daemon returns: speciesId, moves[]{id,pp,...}, itemId, abilityId, nature, level
                moves_raw = pkm.get("moves") or []
                move_ids = [m.get("id", 0) for m in moves_raw] if isinstance(moves_raw, list) else moves_raw
                m = {
                    "speciesID": pkm.get("speciesId", pkm.get("speciesID", pkm.get("num", 0))),
                    "moves": move_ids,
                    "item": pkm.get("itemId", pkm.get("item", 0)),
                    "ability": pkm.get("abilityId", pkm.get("ability", 0)),
                    "nature": pkm.get("nature", 0),
                    "level": pkm.get("level", 50),
                }
                mons.append(m)
            return mons

        data = {
            "battle_id": battle_id,
            "session_id": f"sess_{p1}_{p2}_{int(time.time())}",
            "player_a": p1,
            "player_b": p2,
            "opponent_type": "human",
            "side_a": extract_team(side_a),
            "side_b": extract_team(side_b),
        }
        send_battle_log({
            "event": "battle_init",
            "battle_id": battle_id,
            "session_id": data["session_id"],
            "player_a": p1,
            "player_b": p2,
            "data": data,
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        })

    def log_battle_turn(self, battle_id: str, turn: int, state: dict):
        """Record turn snapshot in daemon format."""
        sides = state.get("battle", state).get("sides", [])
        side_a = sides[0] if len(sides) > 0 else {}
        side_b = sides[1] if len(sides) > 1 else {}
        p1 = side_a.get("name", "Player1")
        p2 = side_b.get("name", "Player2")
        a_active = (side_a.get("pokemons", []) or [])[0] if side_a.get("pokemons") else {}
        b_active = (side_b.get("pokemons", []) or [])[0] if side_b.get("pokemons") else {}

        data = {
            "battle_id": battle_id,
            "session_id": f"sess_{p1}_{p2}",
            "turn": turn,
            "player_a": p1,
            "player_b": p2,
            p1: {"hp": a_active.get("hp", 0), "maxhp": a_active.get("maxhp", 1), "fainted": a_active.get("fainted", False)},
            p2: {"hp": b_active.get("hp", 0), "maxhp": b_active.get("maxhp", 1), "fainted": b_active.get("fainted", False)},
        }
        send_battle_log({
            "event": "turn_executed",
            "battle_id": battle_id,
            "session_id": data["session_id"],
            "turn": turn,
            "player_a": p1,
            "player_b": p2,
            "data": data,
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        })

    def log_battle_result(self, battle: dict):
        """Record battle winner in daemon format."""
        state = battle.get("current_state", {})
        sides = state.get("battle", state).get("sides", [])
        p1 = sides[0].get("name", "Player1") if len(sides) > 0 else "Player1"
        p2 = sides[1].get("name", "Player2") if len(sides) > 1 else "Player2"
        a_alive = sum(1 for p in sides[0].get("pokemons", []) if not p.get("fainted")) if len(sides) > 0 else 0
        b_alive = sum(1 for p in sides[1].get("pokemons", []) if not p.get("fainted")) if len(sides) > 1 else 0
        winner = None
        if a_alive > 0 and b_alive == 0:
            winner = p1
        elif b_alive > 0 and a_alive == 0:
            winner = p2

        data = {
            "battle_id": battle.get("id", ""),
            "session_id": f"sess_{p1}_{p2}",
            "player_a": p1,
            "player_b": p2,
            "result": "completed" if winner else "draw",
            "winner": winner,
            "turns": battle.get("total_turns", 0),
            "side_a_remaining": a_alive,
            "side_b_remaining": b_alive,
        }
        send_battle_log({
            "event": "battle_result",
            "battle_id": battle.get("id", ""),
            "session_id": data["session_id"],
            "player_a": p1,
            "player_b": p2,
            "winner": winner,
            "data": data,
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        })

    # ── Internal ────────────────────────────────

    def _make_entry(self, event: str, data: dict, player_id: str):
        return {
            "event": event,
            "data": data,
            "player_id": player_id or "",
            "timestamp": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        }

    def _extract_init(self, battle_id, state):
        sides = state.get("battle", state).get("sides", [])
        return {
            "battle_id": battle_id,
            "turn": 0,
            "phase": "init",
            "side_a": self._strip_state(sides[0]) if len(sides) > 0 else {},
            "side_b": self._strip_state(sides[1]) if len(sides) > 1 else {},
        }

    def _extract_result(self, battle):
        state = battle.get("current_state", {})
        sides = state.get("battle", state).get("sides", [])
        a_alive = sum(1 for p in sides[0].get("pokemons", []) if not p.get("fainted")) if len(sides) > 0 else 0
        b_alive = sum(1 for p in sides[1].get("pokemons", []) if not p.get("fainted")) if len(sides) > 1 else 0
        winner = None
        if a_alive > 0 and b_alive == 0:
            winner = sides[0].get("name", "A")
        elif b_alive > 0 and a_alive == 0:
            winner = sides[1].get("name", "B")
        return {
            "battle_id": battle.get("id", ""),
            "phase": "result",
            "winner": winner,
            "total_turns": battle.get("total_turns", 0),
            "side_a_remaining": a_alive,
            "side_b_remaining": b_alive,
        }

    @staticmethod
    def _strip_state(side):
        """Keep only analytics-relevant fields, strip heavy event arrays."""
        s = dict(side)
        s.pop("pokemons", None)  # already summarized above
        return s

    # ── Local file buffer ───────────────────────

    def _enqueue(self, entry):
        with self._lock:
            self._buffer.append(entry)
            if len(self._buffer) >= 50:
                self._flush()

    def _flush(self):
        if not self._buffer:
            return
        batch = self._buffer[:]
        self._buffer.clear()
        date_str = datetime.now(timezone.utc).strftime("%Y-%m-%d")
        fpath = self.log_dir / f"events_{date_str}.jsonl"
        try:
            with open(fpath, "a", encoding="utf-8") as f:
                for e in batch:
                    f.write(json.dumps(e, ensure_ascii=False) + "\n")
        except Exception as e:
            logger.error(f"analytics write error: {e}")

    def _start_flush_timer(self):
        if self._flush_timer:
            return
        def _loop():
            while True:
                time.sleep(30)
                with self._lock:
                    self._flush()
        t = threading.Thread(target=_loop, daemon=True)
        t.start()
        self._flush_timer = t


# ── Singleton ──────────────────────────────────

_service: AnalyticsService | None = None


def get_analytics() -> AnalyticsService:
    global _service
    if _service is None:
        from pathlib import Path
        project_root = Path(__file__).resolve().parent.parent.parent
        _service = AnalyticsService(project_root / "logs" / "analytics")
    return _service
