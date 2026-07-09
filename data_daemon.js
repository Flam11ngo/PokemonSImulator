/**
 * Data Daemon — continuously generates battle + UI data for the Kafka pipeline.
 * Battle pool: 70 concurrent workers, each running battles back-to-back.
 * UI pool: 30 concurrent workers, each generating UI events.
 * Usage: node data_daemon.js
 */
const { Battle, Teams, Dex } = require('pokemon-showdown');
const { Kafka } = require('kafkajs');

const BATTLE_WORKERS = 70;
const UI_WORKERS = 30;
const KAFKA_BROKER = process.env.KAFKA_BROKER || "100.107.105.99:9092";

// ── Kafka ──
const kafka = new Kafka({
  brokers: [KAFKA_BROKER],
  connectionTimeout: 5000,
  requestTimeout: 10000,
  retry: { retries: 3 },
});
const producer = kafka.producer();

function ts() { return new Date().toISOString(); }
async function send(topic, key, value) {
  value.timestamp = ts();
  await producer.send({ topic, messages: [{ key: String(key || ""), value: JSON.stringify(value) }] });
}

// ── Game data ──
const allMoves = Dex.moves.all().filter(m =>
  (m.category === "Physical" || m.category === "Special") && m.basePower > 0 && m.num > 0
);
const allSpecies = Dex.species.all().filter(s => s.num > 0 && !s.name.startsWith("MissingNo") && !s.isNonstandard);
const allAbilities = Dex.abilities.all().filter(a => a.num > 0);
const allItems = Dex.items.all().filter(i => i.num > 0 && !i.isBerry && !i.isGem && !i.name.includes("Drive") && !i.name.includes("Memory"));

const NATURES = ["Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed","Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful","Rash","Calm","Gentle","Sassy","Careful","Quirky"];
const NATURE_NAMES = ["Adamant","Jolly","Modest","Timid","Bold","Impish","Calm","Careful"];
const PLAYERS = ["Ash","Serena","Leon","Cynthia","Red","Blue","May","Dawn","Brock","Misty"];
const PAGES = ["/","/matchmaking","/teams","/stats","/data"];
const CLICKS = ["btn_confirm","btn_switch","btn_move_select","btn_join_match","btn_join_bot","btn_save_team","nav_matchmaking","nav_teams","nav_stats","nav_data"];

// ── Team builder (damaging moves only) ──
function randomTeam() {
  const team = [];
  const picked = new Set();
  const count = 3 + Math.floor(Math.random() * 3);
  const pool = allSpecies.filter(() => Math.random() < 0.2).slice(0, count * 4);
  for (const sp of pool) {
    if (picked.has(sp.num) || team.length >= count) continue;
    picked.add(sp.num);
    const movePool = [];
    for (const m of allMoves) {
      if (Dex.data.Learnsets[sp.id]?.learnset?.[m.id]) movePool.push(m);
    }
    const moves = movePool.sort(() => Math.random() - 0.5).slice(0, 4).map(m => m.name);
    if (moves.length < 4) moves.push(...Array(4 - moves.length).fill("Tackle"));
    team.push({
      name: "", species: sp.name,
      ability: allAbilities[Math.floor(Math.random() * allAbilities.length)].name,
      item: allItems[Math.floor(Math.random() * allItems.length)].name,
      moves,
      nature: NATURE_NAMES[Math.floor(Math.random() * NATURE_NAMES.length)],
      evs: { hp: 4, atk: 252, def: 0, spa: 252, spd: 0, spe: 0 },
      ivs: { hp: 31, atk: 31, def: 31, spa: 31, spd: 31, spe: 31 },
      level: 50,
    });
  }
  return team;
}

function teamToInit(team) {
  return team.map(p => ({
    speciesID: Dex.species.get(p.species).num,
    moves: p.moves.map(m => Dex.moves.get(m).num),
    item: Dex.items.get(p.item).num,
    ability: Dex.abilities.get(p.ability).num,
    nature: NATURES.indexOf(p.nature),
    level: 50,
  }));
}

// ── Battle worker ──
let battleCount = 0;
let uiCount = 0;

async function battleWorker(id) {
  while (true) {
    try {
      const bid = `b_${id}_${Date.now().toString(36)}`;
      const teamA = randomTeam();
      const teamB = randomTeam();
      battleCount++;
      const battle = new Battle({ formatid: "gen9customgame", seed: [42, 1, 3, 7], debug: false });
      battle.join("p1", "Player", "Player", Teams.pack(teamA));
      battle.join("p2", "Opponent", "Opponent", Teams.pack(teamB));
      battle.choose("p1", "team 1");
      battle.choose("p2", "team 1");

      await send("battle.logs", bid, {
        event: "battle_init", data: {
          battle_id: bid, opponent_type: "bot",
          side_a: teamToInit(teamA),
          side_b: teamToInit(teamB),
        }
      });

      let turn = 0;
      while (!battle.ended && turn < 50) {
        turn++;
        battle.choose("p1", "move " + (Math.floor(Math.random() * 4) + 1));
        battle.choose("p2", "move " + (Math.floor(Math.random() * 4) + 1));

        const sideA = battle.p1?.active[0];
        const sideB = battle.p2?.active[0];
        await send("battle.logs", bid, {
          event: "turn_executed", data: {
            battle_id: bid, turn,
            side_a: { hp: sideA?.hp || 0, maxhp: sideA?.maxhp || 1, fainted: sideA?.fainted || false },
            side_b: { hp: sideB?.hp || 0, maxhp: sideB?.maxhp || 1, fainted: sideB?.fainted || false },
          }
        });

        // Parse damage/faint/switch from recent log lines
        const from = Math.max(0, battle.log.length - 30);
        for (let i = from; i < battle.log.length; i++) {
          const line = battle.log[i];
          if (line.startsWith("|-damage|")) {
            const p = line.split("|");
            const hp = (p[2] || "").split(" ")[0].split("/");
            const side = p[1].startsWith("p1") ? "a" : "b";
            const dmg = Math.max(0, (parseInt(hp[1]) || 0) - (parseInt(hp[0]) || 0));
            if (dmg > 0) await send("battle.logs", bid, { event: "turn_damage", data: { battle_id: bid, turn, target_side: side, damage: dmg, fainted: (p[2] || "").includes("fnt") } });
          } else if (line.startsWith("|faint|")) {
            await send("battle.logs", bid, { event: "turn_faint", data: { battle_id: bid, turn, side: line.includes("p1") ? "a" : "b" } });
          } else if (line.startsWith("|switch|") || line.startsWith("|drag|")) {
            await send("battle.logs", bid, { event: "turn_switch", data: { battle_id: bid, turn, side: line.includes("p1") ? "a" : "b", reason: line.startsWith("|drag|") ? "forced" : "manual" } });
          }
        }

        // Force switches
        for (const p of ["p1", "p2"]) {
          if (battle[p]?.activeRequest?.forceSwitch?.[0]) {
            const pokemon = battle[p]?.pokemon?.filter(pk => !pk.fainted && pk.position !== battle[p].active[0]?.position);
            if (pokemon?.length) battle.choose(p, "switch " + (pokemon[0].position + 1));
            else battle.choose(p, "pass");
          }
        }
      }

      const aRem = battle.p1?.pokemon?.filter(p => !p.fainted).length || 0;
      const bRem = battle.p2?.pokemon?.filter(p => !p.fainted).length || 0;
      let winner = null, result = "draw";
      if (aRem > 0 && bRem === 0) { winner = "a"; result = "completed"; }
      else if (bRem > 0 && aRem === 0) { winner = "b"; result = "completed"; }

      await send("battle.logs", bid, {
        event: "battle_result", data: { battle_id: bid, result, winner, turns: turn, side_a_remaining: aRem, side_b_remaining: bRem }
      });
    } catch (e) {
      // silently retry next battle
    }
  }
}

// ── UI worker ──
async function uiWorker(id) {
  while (true) {
    try {
      uiCount++;
      const player = PLAYERS[Math.floor(Math.random() * PLAYERS.length)];
      const r = Math.random();
      if (r < 0.2) {
        await send("player.ui.events", player, { event: "page_view", player_id: player, data: { page: PAGES[Math.floor(Math.random() * PAGES.length)] } });
      } else if (r < 0.4) {
        await send("player.ui.events", player, { event: "ui_click", player_id: player, data: { element: CLICKS[Math.floor(Math.random() * CLICKS.length)] } });
      } else if (r < 0.55) {
        await send("player.ui.events", player, { event: "player_state", player_id: player, data: { from: "idle", to: ["matching", "battling", "teambuilding"][Math.floor(Math.random() * 3)] } });
      } else if (r < 0.7) {
        await send("player.ui.events", player, { event: "matchmaking_join", player_id: player, data: { opponent_type: Math.random() < 0.5 ? "human" : "bot" } });
      } else if (r < 0.85) {
        await send("player.ui.events", player, { event: "session_start", player_id: player, data: { player_id: player } });
      } else {
        const team = Array.from({ length: Math.floor(Math.random() * 4) + 3 }, () => {
          const sp = allSpecies[Math.floor(Math.random() * allSpecies.length)];
          return { speciesID: sp.num, moves: [], item: allItems[Math.floor(Math.random() * allItems.length)].num, ability: allAbilities[Math.floor(Math.random() * allAbilities.length)].num, nature: Math.floor(Math.random() * 25) };
        });
        await send("player.ui.events", player, { event: "team_save", player_id: player, data: { team_name: ["晴天队", "雨天队", "沙暴队", "龙队", "平衡队", "空间队", "强化队"][Math.floor(Math.random() * 7)], pokemon_count: team.length, pokemon: team } });
      }
      await new Promise(r => setTimeout(r, 200 + Math.random() * 800)); // 0.2-1s interval
    } catch (e) { /* retry */ }
  }
}

// ── Main ──
async function main() {
  await producer.connect();
  console.log(`Data daemon: ${BATTLE_WORKERS} battle + ${UI_WORKERS} UI workers → ${KAFKA_BROKER}`);

  // Connection test
  try {
    await send("battle.logs", "ping", { event: "daemon_start", data: { workers: { battle: BATTLE_WORKERS, ui: UI_WORKERS } } });
    console.log("Kafka connection OK");
  } catch (e) {
    console.error("Kafka connection FAILED:", e.message);
  }

  const workers = [];
  for (let i = 0; i < BATTLE_WORKERS; i++) workers.push(battleWorker(i + 1));
  for (let i = 0; i < UI_WORKERS; i++) workers.push(uiWorker(i + 1));

  // Heartbeat with stats
  let uptime = 0;
  setInterval(() => {
    uptime++;
    process.stdout.write(`\r[${uptime}s] battles:${battleCount} ui:${uiCount}  workers:${BATTLE_WORKERS}b+${UI_WORKERS}u  `);
  }, 1000);

  await Promise.all(workers);
}

main().catch(e => { console.error(e); process.exit(1); });
