#!/usr/bin/env python3
import argparse
import concurrent.futures
import json
import os
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Dict, List, Optional, Tuple

POKEAPI_BASE = "https://pokeapi.co/api/v2"


def fetch_json(url: str, timeout: int, retry: int) -> Optional[dict]:
    delay = 0.5
    for attempt in range(retry + 1):
        try:
            with urllib.request.urlopen(url, timeout=timeout) as resp:
                return json.loads(resp.read().decode("utf-8"))
        except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError):
            if attempt == retry:
                return None
            time.sleep(delay)
            delay = min(delay * 2.0, 4.0)
    return None


def fetch_paginated(endpoint: str, timeout: int, retry: int) -> List[dict]:
    limit = 200
    offset = 0
    results: List[dict] = []

    while True:
        url = f"{POKEAPI_BASE}/{endpoint}?limit={limit}&offset={offset}"
        payload = fetch_json(url, timeout, retry)
        if not payload:
            break
        page = payload.get("results", [])
        if not page:
            break
        results.extend(page)
        if not payload.get("next"):
            break
        offset += limit

    return results


def parse_id_from_url(url: str) -> int:
    path = urllib.parse.urlparse(url).path.rstrip("/")
    return int(path.split("/")[-1])


def slug_to_name(slug: str) -> str:
    out = []
    upper = True
    for ch in slug:
        if ch == "-":
            out.append(" ")
            upper = True
            continue
        out.append(ch.upper() if upper else ch)
        upper = False
    return "".join(out)


def map_move_effect(move_payload: dict) -> Tuple[str, int, int, int]:
    move_name = move_payload.get("name", "")
    meta = move_payload.get("meta") or {}
    ailment = ((meta.get("ailment") or {}).get("name") or "none").lower()
    ailment_chance = int((meta.get("ailment_chance") or 0) or 0)
    flinch_chance = int((meta.get("flinch_chance") or 0) or 0)
    drain = int((meta.get("drain") or 0) or 0)

    if drain > 0:
        return ("Drain", int(drain), 0, int(move_payload.get("effect_chance") or 100))
    if drain < 0:
        return ("Recoil", int(-drain), 0, int(move_payload.get("effect_chance") or 100))
    if flinch_chance > 0:
        return ("Flinch", 0, 0, flinch_chance)

    if ailment in ("burn",):
        return ("Burn", 0, 0, ailment_chance or int(move_payload.get("effect_chance") or 100))
    if ailment in ("freeze",):
        return ("Freeze", 0, 0, ailment_chance or int(move_payload.get("effect_chance") or 100))
    if ailment in ("paralysis",):
        return ("Paralyze", 0, 0, ailment_chance or int(move_payload.get("effect_chance") or 100))
    if ailment in ("sleep",):
        return ("Sleep", 0, 0, ailment_chance or int(move_payload.get("effect_chance") or 100))
    if ailment in ("poison", "bad-poison"):
        is_toxic = (move_name == "toxic")
        return (
            "Poison",
            2 if is_toxic else 0,
            0,
            ailment_chance or int(move_payload.get("effect_chance") or 100),
        )

    stat_changes = move_payload.get("stat_changes") or []
    if stat_changes:
        first = stat_changes[0]
        stat_name = ((first.get("stat") or {}).get("name") or "").lower()
        stage = int(first.get("change") or 0)
        idx_map = {
            "attack": 1,
            "defense": 2,
            "special-attack": 3,
            "special-defense": 4,
            "speed": 5,
        }
        idx = idx_map.get(stat_name, 0)
        if idx > 0 and stage != 0:
            target_name = ((move_payload.get("target") or {}).get("name") or "").lower()
            affect_self = target_name in ("user", "users-field", "user-and-allies")
            encoded_idx = -idx if affect_self else idx
            chance = int(move_payload.get("effect_chance") or 100)
            return ("StatChange", encoded_idx, stage, chance)

    if move_name == "protect":
        return ("Safeguard", 0, 0, 100)

    return ("None", 0, 0, int(move_payload.get("effect_chance") or 0))


def map_mapped_ability_type(api_name: str) -> str:
    known = {
        "blaze",
        "torrent",
        "overgrow",
        "intimidate",
        "multiscale",
    }
    return api_name if api_name in known else "none"


def fetch_details(entries: List[dict], workers: int, timeout: int, retry: int) -> List[dict]:
    out: List[dict] = []

    def task(entry: dict) -> Optional[dict]:
        return fetch_json(entry["url"], timeout, retry)

    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as ex:
        futures = [ex.submit(task, e) for e in entries]
        for fut in concurrent.futures.as_completed(futures):
            payload = fut.result()
            if payload:
                out.append(payload)

    return out


def build_moves_runtime(raw_moves: List[dict]) -> dict:
    moves = []
    for m in raw_moves:
        effect, p1, p2, chance = map_move_effect(m)
        move_id = int(m.get("id") or 0)
        moves.append(
            {
                "id": move_id,
                "name": slug_to_name(m.get("name", "")),
                "apiName": m.get("name", ""),
                "description": "",
                "type": ((m.get("type") or {}).get("name") or "normal").lower(),
                "category": ((m.get("damage_class") or {}).get("name") or "status").lower(),
                "power": int((m.get("power") or 0) or 0),
                "accuracy": int((m.get("accuracy") or 100) or 100),
                "pp": int((m.get("pp") or 0) or 0),
                "effect": effect,
                "effectChance": chance,
                "effectParam1": p1,
                "effectParam2": p2,
                "priority": int((m.get("priority") or 0) or 0),
                "target": ((m.get("target") or {}).get("name") or "selected-pokemon"),
                "damageClass": ((m.get("damage_class") or {}).get("name") or "status").lower(),
            }
        )

    moves.sort(key=lambda x: x["id"])
    return {"moves": moves}


def build_abilities_runtime(raw_abilities: List[dict]) -> dict:
    abilities = []
    for a in raw_abilities:
        desc = ""
        for entry in a.get("effect_entries", []):
            lang = ((entry.get("language") or {}).get("name") or "")
            if lang == "en":
                desc = entry.get("short_effect") or entry.get("effect") or ""
                break

        api_name = (a.get("name") or "").lower()
        abilities.append(
            {
                "id": int(a.get("id") or 0),
                "name": slug_to_name(api_name),
                "apiName": api_name,
                "description": desc,
                "mappedType": map_mapped_ability_type(api_name),
            }
        )

    abilities.sort(key=lambda x: x["id"])
    return {"abilities": abilities}


def build_evolution_cache(raw_species: List[dict], timeout: int, retry: int, workers: int) -> Dict[int, dict]:
    chain_urls = {}
    for s in raw_species:
        chain_url = ((s.get("evolution_chain") or {}).get("url") or "")
        if chain_url:
            chain_id = parse_id_from_url(chain_url)
            chain_urls[chain_id] = chain_url

    payloads: Dict[int, dict] = {}

    def task(item: Tuple[int, str]) -> Tuple[int, Optional[dict]]:
        cid, url = item
        return cid, fetch_json(url, timeout, retry)

    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as ex:
        futures = [ex.submit(task, it) for it in chain_urls.items()]
        for fut in concurrent.futures.as_completed(futures):
            cid, payload = fut.result()
            if payload:
                payloads[cid] = payload

    return payloads


def flatten_chain(chain_node: dict) -> Dict[str, Tuple[int, int]]:
    mapping: Dict[str, Tuple[int, int]] = {}

    def walk(node: dict):
        name = ((node.get("species") or {}).get("name") or "")
        evolves = node.get("evolves_to") or []
        if not evolves:
            mapping[name] = (-1, 0)
            return

        next_species = evolves[0]
        next_name = ((next_species.get("species") or {}).get("name") or "")
        level = 0
        details = next_species.get("evolution_details") or []
        if details:
            level = int((details[0].get("min_level") or 0) or 0)
        mapping[name] = (0, level)

        for child in evolves:
            walk(child)

    walk(chain_node)

    # fill real next id later in caller
    return mapping


def build_species_runtime(raw_pokemon: List[dict], raw_species_meta: List[dict], evo_payloads: Dict[int, dict]) -> dict:
    species_meta_by_name = {s.get("name", ""): s for s in raw_species_meta}
    species_by_name = {s.get("name", ""): s for s in raw_pokemon}
    species = []

    chain_map_by_name: Dict[str, Tuple[str, int]] = {}
    for payload in evo_payloads.values():
        chain = payload.get("chain") or {}
        flat = flatten_chain(chain)

        def fill(node: dict):
            cur_name = ((node.get("species") or {}).get("name") or "")
            evolves = node.get("evolves_to") or []
            if evolves:
                child = evolves[0]
                next_name = ((child.get("species") or {}).get("name") or "")
                level = 0
                details = child.get("evolution_details") or []
                if details:
                    level = int((details[0].get("min_level") or 0) or 0)
                chain_map_by_name[cur_name] = (next_name, level)
            else:
                chain_map_by_name[cur_name] = ("", 0)
            for c in evolves:
                fill(c)

        fill(chain)

    for s in raw_pokemon:
        sid = int(s.get("id") or 0)
        name = s.get("name") or ""
        meta = species_meta_by_name.get(name, {})
        types = sorted(s.get("types") or [], key=lambda x: int(x.get("slot") or 0))
        type1 = ((types[0].get("type") or {}).get("name") if types else "normal") or "normal"
        type2 = ((types[1].get("type") or {}).get("name") if len(types) > 1 else "count") or "count"

        stat_map = {((st.get("stat") or {}).get("name") or ""): int(st.get("base_stat") or 0) for st in (s.get("stats") or [])}
        base_stats = [
            stat_map.get("hp", 1),
            stat_map.get("attack", 1),
            stat_map.get("defense", 1),
            stat_map.get("special-attack", 1),
            stat_map.get("special-defense", 1),
            stat_map.get("speed", 1),
        ]

        egg_groups = [((g.get("name") or "").replace("-", " ")) for g in (meta.get("egg_groups") or [])]

        ability_ids = []
        hidden_ability_id = 0
        for ab in s.get("abilities") or []:
            aid = parse_id_from_url(((ab.get("ability") or {}).get("url") or "0"))
            ability_ids.append(aid)
            if ab.get("is_hidden"):
                hidden_ability_id = aid

        gender_rate = int((meta.get("gender_rate") or 0) or 0)
        male_ratio = -1.0 if gender_rate < 0 else (8.0 - float(gender_rate)) / 8.0

        next_name, evo_level = chain_map_by_name.get(name, ("", 0))
        next_id = int((species_by_name.get(next_name) or {}).get("id") or -1) if next_name else -1

        move_ids = []
        seen = set()
        for mv in s.get("moves") or []:
            murl = ((mv.get("move") or {}).get("url") or "")
            if not murl:
                continue
            mid = parse_id_from_url(murl)
            if mid not in seen:
                seen.add(mid)
                move_ids.append(mid)

        species.append(
            {
                "id": sid,
                "name": slug_to_name(name),
                "height": int((s.get("height") or 0) or 0),
                "weight": int((s.get("weight") or 0) or 0),
                "type1": type1,
                "type2": type2,
                "baseStats": base_stats,
                "eggGroups": egg_groups,
                "abilities": ability_ids,
                "hiddenAbilityID": hidden_ability_id,
                "maleRatio": male_ratio,
                "nextEvolutionID": next_id,
                "evolutionLevel": evo_level,
                "learnableMoves": move_ids,
            }
        )

    species.sort(key=lambda x: x["id"])
    return {"species": species}


def write_json(path: str, payload: dict):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2, ensure_ascii=False)


def trim_entries(entries: List[dict], limit: int) -> List[dict]:
    if limit <= 0:
        return entries
    return entries[:limit]


def main():
    parser = argparse.ArgumentParser(description="Fetch move/ability/species data from PokeAPI")
    parser.add_argument("--limit", type=int, default=0, help="limit count for each endpoint (0 = all)")
    parser.add_argument("--workers", type=int, default=12, help="concurrent workers")
    parser.add_argument("--retry", type=int, default=3, help="retry count per request")
    parser.add_argument("--timeout", type=int, default=20, help="request timeout seconds")
    parser.add_argument("--output-dir", type=str, default="data", help="output directory")
    parser.add_argument("--overwrite-runtime", action="store_true", help="overwrite data/moves.json, abilities.json, species.json")
    args = parser.parse_args()

    start = time.time()

    move_entries = trim_entries(fetch_paginated("move", args.timeout, args.retry), args.limit)
    ability_entries = trim_entries(fetch_paginated("ability", args.timeout, args.retry), args.limit)
    pokemon_entries = trim_entries(fetch_paginated("pokemon", args.timeout, args.retry), args.limit)
    species_entries = trim_entries(fetch_paginated("pokemon-species", args.timeout, args.retry), args.limit)

    print(
        f"discovered moves={len(move_entries)} abilities={len(ability_entries)} "
        f"pokemon={len(pokemon_entries)} species={len(species_entries)}"
    )

    raw_moves = fetch_details(move_entries, args.workers, args.timeout, args.retry)
    raw_abilities = fetch_details(ability_entries, args.workers, args.timeout, args.retry)
    raw_pokemon = fetch_details(pokemon_entries, args.workers, args.timeout, args.retry)
    raw_species = fetch_details(species_entries, args.workers, args.timeout, args.retry)

    raw_moves.sort(key=lambda x: int(x.get("id") or 0))
    raw_abilities.sort(key=lambda x: int(x.get("id") or 0))
    raw_pokemon.sort(key=lambda x: int(x.get("id") or 0))
    raw_species.sort(key=lambda x: int(x.get("id") or 0))

    evo_payloads = build_evolution_cache(raw_species, args.timeout, args.retry, args.workers)

    runtime_moves = build_moves_runtime(raw_moves)
    runtime_abilities = build_abilities_runtime(raw_abilities)
    runtime_species = build_species_runtime(raw_pokemon, raw_species, evo_payloads)

    out = args.output_dir
    write_json(os.path.join(out, "raw_moves.json"), {"moves": raw_moves})
    write_json(os.path.join(out, "raw_abilities.json"), {"abilities": raw_abilities})
    write_json(os.path.join(out, "raw_pokemon.json"), {"pokemon": raw_pokemon})
    write_json(os.path.join(out, "raw_species.json"), {"species": raw_species})

    write_json(os.path.join(out, "fetched_moves.json"), runtime_moves)
    write_json(os.path.join(out, "fetched_abilities.json"), runtime_abilities)
    write_json(os.path.join(out, "fetched_species.json"), runtime_species)

    if args.overwrite_runtime:
        write_json(os.path.join(out, "moves.json"), runtime_moves)
        write_json(os.path.join(out, "abilities.json"), runtime_abilities)
        write_json(os.path.join(out, "species.json"), runtime_species)

    print(
        "done in %.1fs: raw(moves=%d abilities=%d pokemon=%d species=%d), runtime(moves=%d abilities=%d species=%d)"
        % (
            time.time() - start,
            len(raw_moves),
            len(raw_abilities),
            len(raw_pokemon),
            len(raw_species),
            len(runtime_moves["moves"]),
            len(runtime_abilities["abilities"]),
            len(runtime_species["species"]),
        )
    )


if __name__ == "__main__":
    main()
