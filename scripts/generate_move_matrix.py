#!/usr/bin/env python3
import csv
import json
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
MOVES_JSON = REPO_ROOT / "data" / "moves.json"
DOCS_DIR = REPO_ROOT / "docs"
CSV_OUT = DOCS_DIR / "moves-progress.csv"
MD_OUT = DOCS_DIR / "moves-progress.md"

# Name-level handlers currently wired in Battle.cpp / MoveEffectHandler.cpp.
IMPLEMENTED_NAME_RULES = {
    "batonpass",
    "dig",
    "earthquake",
    "encore",
    "heavyslam",
    "knockoff",
    "pursuit",
    "round",
    "haze",
    "recover",
    "softboiled",
    "milkdrink",
    "reflect",
    "lightscreen",
    "leechseed",
    "substitute",
    "sandattack",
    "smokescreen",
    "flash",
    "trickroom",
    "uturn",
    "weatherball",
    "dragondance",
}

# Effect-level handlers currently wired in MoveEffectHandler.cpp + parser aliases in MoveData.cpp.
IMPLEMENTED_EFFECTS = {
    "burn",
    "confuse",
    "drain",
    "flinch",
    "freeze",
    "paralyze",
    "poison",
    "recoil",
    "safeguard",
    "sleep",
    "statchange",
}

MULTI_TARGET_VALUES = {
    "all-pokemon",
    "all-allies",
    "all-opponents",
    "everyone",
    "users-field",
    "user-and-allies",
}


def normalize_name(value: str) -> str:
    if not value:
        return ""
    return "".join(ch for ch in value.lower() if ch.isalnum())


def classify_move(move: dict) -> tuple[str, str]:
    name = move.get("name", "")
    api_name = move.get("apiName", "")
    category = (move.get("category", "") or "").strip().lower()
    effect = normalize_name(str(move.get("effect", "none")))
    target = (move.get("target", "") or "").strip().lower()

    n_name = normalize_name(name)
    n_api_name = normalize_name(api_name)

    if n_name in IMPLEMENTED_NAME_RULES or n_api_name in IMPLEMENTED_NAME_RULES:
        status = "implemented"
        reason = "hardcoded name rule present"
    elif effect in IMPLEMENTED_EFFECTS:
        status = "implemented"
        reason = f"effect handler present: {effect}"
    elif category in {"physical", "special"} and effect in {"", "none"}:
        status = "partial"
        reason = "base damage path likely works; special mechanic not confirmed"
    else:
        status = "todo"
        if effect not in {"", "none"}:
            reason = f"effect handler missing: {effect}"
        else:
            reason = "no explicit mechanic handler"

    if target in MULTI_TARGET_VALUES and status == "implemented":
        status = "partial"
        reason = "multi-target semantic may differ in singles-first mode"

    return status, reason


def load_moves() -> list[dict]:
    root = json.loads(MOVES_JSON.read_text(encoding="utf-8"))
    moves = root.get("moves", [])
    if not isinstance(moves, list):
        raise ValueError("data/moves.json: root.moves must be an array")
    return moves


def write_outputs(rows: list[dict], summary: Counter, next_batch: list[dict]) -> None:
    DOCS_DIR.mkdir(parents=True, exist_ok=True)

    with CSV_OUT.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "id",
                "name",
                "apiName",
                "category",
                "effect",
                "target",
                "status",
                "reason",
            ],
        )
        writer.writeheader()
        writer.writerows(rows)

    generated_at = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%SZ")
    lines = [
        "# Gen9 Move Progress Matrix (Singles 1v1 First)",
        "",
        f"Generated at: {generated_at}",
        "",
        "Scope:",
        "Showdown Gen9 full move scope (including non-standard/historical), implementation staged in singles 1v1 first.",
        "",
        "## Summary",
        "",
        f"- Total moves in data/moves.json: {summary['total']}",
        f"- Implemented: {summary['implemented']}",
        f"- Partial: {summary['partial']}",
        f"- Todo: {summary['todo']}",
        "",
        "## Heuristic Rules Used",
        "",
        "- `implemented`: explicit move name rule exists, or effect handler exists.",
        "- `partial`: baseline damage path only, or multi-target semantics not fully verified in singles-first mode.",
        "- `todo`: no dedicated handler detected.",
        "",
        "## Current Next Batch (Top 80 Todo By ID)",
        "",
        "| id | name | apiName | category | effect | reason |",
        "| --- | --- | --- | --- | --- | --- |",
    ]

    for m in next_batch:
        lines.append(
            f"| {m['id']} | {m['name']} | {m['apiName']} | {m['category']} | {m['effect']} | {m['reason']} |"
        )

    lines += [
        "",
        "## Files",
        "",
        "- Full matrix CSV: docs/moves-progress.csv",
        "- Summary markdown: docs/moves-progress.md",
        "",
        "## Regenerate",
        "",
        "```bash",
        "python3 scripts/generate_move_matrix.py",
        "```",
        "",
    ]

    MD_OUT.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    moves = load_moves()

    rows: list[dict] = []
    summary = Counter()

    for move in moves:
        status, reason = classify_move(move)
        row = {
            "id": move.get("id", 0),
            "name": move.get("name", ""),
            "apiName": move.get("apiName", ""),
            "category": move.get("category", ""),
            "effect": move.get("effect", ""),
            "target": move.get("target", ""),
            "status": status,
            "reason": reason,
        }
        rows.append(row)
        summary[status] += 1
        summary["total"] += 1

    rows.sort(key=lambda x: int(x["id"]))
    todo_rows = [r for r in rows if r["status"] == "todo"]
    next_batch = todo_rows[:80]

    write_outputs(rows, summary, next_batch)

    print(f"Generated: {CSV_OUT}")
    print(f"Generated: {MD_OUT}")
    print(
        "Summary => "
        f"total={summary['total']}, "
        f"implemented={summary['implemented']}, "
        f"partial={summary['partial']}, "
        f"todo={summary['todo']}"
    )


if __name__ == "__main__":
    main()
