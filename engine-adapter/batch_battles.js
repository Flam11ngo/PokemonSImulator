/**
 * Batch battle runner — 100 rapid bot-vs-bot battles with damaging moves only.
 * Uses pokemon-showdown Battle class directly (no WebSocket, no subprocess).
 * Sends structured logs to Kafka.
 * Usage: node batch_battles.js [count] [battlesPerSec]
 */
const { Battle, Teams, Dex } = require('pokemon-showdown');
const { Kafka } = require('kafkajs');

const BATTLE_COUNT = parseInt(process.argv[2]) || 100;
const RATE = parseInt(process.argv[3]) || 10; // battles per second
const KAFKA_BROKER = "100.107.105.99:9092";

// ── Setup ──
const kafka = new Kafka({ brokers: [KAFKA_BROKER] });
const producer = kafka.producer();

function ts() { return new Date().toISOString(); }

async function send(topic, key, value) {
  value.timestamp = ts();
  await producer.send({ topic, messages: [{ key: String(key), value: JSON.stringify(value) }] });
}

// ── Species with only damaging moves ──
const DAMAGING_CATEGORIES = new Set(["Physical", "Special"]);
const allMoves = Dex.moves.all().filter(m => DAMAGING_CATEGORIES.has(m.category) && m.basePower > 0 && m.num > 0);
const allSpecies = Dex.species.all().filter(s => s.num > 0 && !s.name.startsWith("MissingNo") && !s.isNonstandard);
const allAbilities = Dex.abilities.all().filter(a => a.num > 0);
const allItems = Dex.items.all().filter(i => i.num > 0 && i.isBerry !== true && i.isGem !== true && !i.name.includes("Drive") && !i.name.includes("Memory") && i.name !== "Light Ball");

function randomTeam() {
  const team = [];
  const picked = new Set();
  const count = 3 + Math.floor(Math.random() * 3); // 3-5 Pokemon
  const candidates = allSpecies.filter(() => Math.random() < 0.3).slice(0, count * 3);
  for (const sp of candidates.slice(0, count)) {
    if (picked.has(sp.num)) continue;
    picked.add(sp.num);
    const moves = [];
    const movePool = [];
    for (const m of allMoves) {
      if (Dex.data.Learnsets[sp.id]?.learnset?.[m.id]) movePool.push(m);
    }
    const selected = movePool.sort(() => Math.random() - 0.5).slice(0, 4);
    for (const m of selected) moves.push(m.name);
    if (moves.length < 4) moves.push("Tackle"); // fallback
    team.push({
      name: "", species: sp.name,
      ability: allAbilities[Math.floor(Math.random() * allAbilities.length)].name,
      item: allItems[Math.floor(Math.random() * allItems.length)].name,
      moves,
      nature: ["Adamant","Jolly","Modest","Timid","Bold","Impish","Calm","Careful"][Math.floor(Math.random() * 8)],
      evs: { hp: 4, atk: 252, def: 0, spa: 252, spd: 0, spe: 0 },
      ivs: { hp: 31, atk: 31, def: 31, spa: 31, spd: 31, spe: 31 },
      level: 50,
    });
  }
  return team;
}

// ── Battle runner ──
async function runBattle(battleId) {
  const teamA = randomTeam();
  const teamB = randomTeam();
  const aPack = Teams.pack(teamA);
  const bPack = Teams.pack(teamB);

  const battle = new Battle({ formatid: "gen9customgame", seed: [42, 1, 3, 7], debug: false });
  battle.join("p1", "Player", "Player", aPack);
  battle.join("p2", "Opponent", "Opponent", bPack);
  battle.choose("p1", "team 1");
  battle.choose("p2", "team 1");

  // Send init
  await send("battle.logs", battleId, {
    event: "battle_init", data: {
      battle_id: battleId,
      side_a: teamA.map((p, i) => ({ speciesID: Dex.species.get(p.species).num, moves: p.moves.map(m => Dex.moves.get(m).num), item: Dex.items.get(p.item).num, ability: Dex.abilities.get(p.ability).num, nature: ["Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"].indexOf(p.nature), level: 50 })),
      side_b: teamB.map((p, i) => ({ speciesID: Dex.species.get(p.species).num, moves: p.moves.map(m => Dex.moves.get(m).num), item: Dex.items.get(p.item).num, ability: Dex.abilities.get(p.ability).num, nature: ["Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"].indexOf(p.nature), level: 50 })),
      opponent_type: "bot",
    }
  });

  // Turn loop
  let turn = 0;
  while (!battle.ended && turn < 50) {
    turn++;
    const a1 = "move " + (Math.floor(Math.random() * 4) + 1);
    const a2 = "move " + (Math.floor(Math.random() * 4) + 1);
    battle.choose("p1", a1);
    battle.choose("p2", a2);

    // Parse events from battle log
    const events = [];
    let logIdx = battle.log.length - 20; // approximate
    if (logIdx < 0) logIdx = 0;

    for (let i = logIdx; i < battle.log.length; i++) {
      const line = battle.log[i];
      if (line.startsWith("|-damage|")) {
        const parts = line.split("|");
        const raw = parts[2] || "";
        const hp = raw.split(" ")[0].split("/");
        const side = parts[1].startsWith("p1") ? "a" : "b";
        const dmg = Math.max(0, parseInt(hp[1]) - parseInt(hp[0]));
        if (dmg > 0) {
          events.push({ event: "turn_damage", data: { target_side: side, damage: dmg, fainted: raw.includes("fnt") } });
        }
      } else if (line.startsWith("|faint|")) {
        events.push({ event: "turn_faint", data: { side: line.includes("p1") ? "a" : "b" } });
      } else if (line.startsWith("|switch|") || line.startsWith("|drag|")) {
        events.push({ event: "turn_switch", data: { side: line.includes("p1") ? "a" : "b", reason: line.startsWith("|drag|") ? "faint_replace" : "manual" } });
      }
    }

    // Send turn log
    const sideA = battle.p1?.active[0];
    const sideB = battle.p2?.active[0];
    await send("battle.logs", battleId, {
      event: "turn_executed", data: {
        battle_id: battleId, turn,
        side_a: { hp: sideA?.hp || 0, maxhp: sideA?.maxhp || 1, fainted: sideA?.fainted || false },
        side_b: { hp: sideB?.hp || 0, maxhp: sideB?.maxhp || 1, fainted: sideB?.fainted || false },
      }
    });

    for (const ev of events) {
      ev.data.battle_id = battleId;
      ev.data.turn = turn;
      await send("battle.logs", battleId, ev);
    }

    // Handle force switches
    while (!battle.ended && (battle.p1?.activeRequest?.forceSwitch?.[0] || battle.p2?.activeRequest?.forceSwitch?.[0])) {
      if (battle.p1?.activeRequest?.forceSwitch?.[0]) {
        const bench = battle.p1?.pokemon?.filter(p => !p.fainted && p.position !== battle.p1.active[0]?.position);
        const choice = bench?.length ? "switch " + (bench[0].position + 1) : "pass";
        battle.choose("p1", choice);
      }
      if (battle.p2?.activeRequest?.forceSwitch?.[0]) {
        const bench = battle.p2?.pokemon?.filter(p => !p.fainted && p.position !== battle.p2.active[0]?.position);
        const choice = bench?.length ? "switch " + (bench[0].position + 1) : "pass";
        battle.choose("p2", choice);
      }
    }
  }

  // Result
  const aRem = battle.p1?.pokemon?.filter(p => !p.fainted).length || 0;
  const bRem = battle.p2?.pokemon?.filter(p => !p.fainted).length || 0;
  let winner = null, result = "draw";
  if (aRem > 0 && bRem === 0) { winner = "Player"; result = "completed"; }
  else if (bRem > 0 && aRem === 0) { winner = "Opponent"; result = "completed"; }

  await send("battle.logs", battleId, {
    event: "battle_result", data: {
      battle_id: battleId, result, winner, turns: turn,
      own_remaining: aRem, opp_remaining: bRem,
    }
  });
}

// ── Worker pool ──
async function workerPool(total, concurrency) {
  const results = [];
  const queue = Array.from({ length: total }, (_, i) => i);
  let completed = 0;

  async function worker() {
    while (queue.length) {
      const i = queue.shift();
      if (i === undefined) break;
      const bid = `batch_${i}_${Date.now().toString(36)}`;
      try {
        await runBattle(bid);
        completed++;
        process.stdout.write(`\r${completed}/${total} done`);
      } catch (e) {
        console.error(`\nBattle ${bid} error:`, e.message);
      }
    }
  }

  const start = Date.now();
  const workers = Array.from({ length: concurrency }, () => worker());
  await Promise.all(workers);

  const elapsed = ((Date.now() - start) / 1000).toFixed(1);
  console.log(`\nAll ${total} battles complete in ${elapsed}s (${concurrency} concurrent).`);
}

// ── Main ──
async function main() {
  await producer.connect();
  const concurrency = RATE; // RATE param reused as concurrency
  console.log(`Starting ${BATTLE_COUNT} battles, ${concurrency} concurrent...`);
  await workerPool(BATTLE_COUNT, concurrency);
  await producer.disconnect();
}

main().catch(e => { console.error(e); process.exit(1); });
