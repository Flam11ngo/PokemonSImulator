const fs = require('fs');
const dir = 'E:/PokemonSImulator/logs/analytics';
fs.mkdirSync(dir, { recursive: true });

function id() { return Math.random().toString(36).slice(2,6); }
function pick(arr, n) { if (n) { const r = []; for (let i = 0; i < n; i++) r.push(arr[Math.floor(Math.random() * arr.length)]); return r; } return arr[Math.floor(Math.random() * arr.length)]; }

const SPECIES = [
  {id:25},{id:6},{id:9},{id:3},{id:445},{id:887},{id:983},{id:730},
  {id:146},{id:149},{id:248},{id:448},{id:658},{id:778},{id:823},{id:901}
];
const MOVES = [
  {id:85,name:'Thunderbolt'},{id:89,name:'Earthquake'},{id:94,name:'Flamethrower'},
  {id:57,name:'Surf'},{id:247,name:'Shadow Ball'},{id:369,name:'U-turn'},
  {id:14,name:'Swords Dance'},{id:182,name:'Protect'},{id:394,name:'Dragon Pulse'},
  {id:370,name:'Close Combat'},{id:188,name:'Stealth Rock'},{id:59,name:'Ice Beam'}
];
const ITEMS = [211,184,185,219,220,221,232,234,275,287,297,188,234,236,240];
const ABILITIES = [9,3,26,66,168,45,146,67,10,104,145,186,5,55,125];
const NATURES = [3,10,15,5,8,13,20,23,0];
const PLAYERS = ['Ash', 'Serena', 'Leon', 'Cynthia'];
const SESSIONS = [id(), id(), id(), id()];
const PAGES = ['/', '/matchmaking', '/teams', '/stats', '/data'];
const CLICKS = ['btn_confirm','btn_switch','btn_move_select','btn_join_match','btn_join_bot',
  'btn_cancel_match','btn_leave_battle','btn_reset','btn_save_team',
  'nav_matchmaking','nav_teams','nav_stats','nav_data'];
const STATES = ['idle', 'matching', 'battling', 'teambuilding', 'idle'];
const ABILITY_NAMES = ['Intimidate','Swift Swim','Drought','Mold Breaker','Regenerator','Levitate','Prankster'];

let events = [];
const now = Date.now();

// Session starts
for (let i = 0; i < 4; i++)
  events.push({event:'session_start', data:{player_id:PLAYERS[i]}, session_id:SESSIONS[i], player_id:PLAYERS[i], timestamp:new Date(now - (4-i)*7200000).toISOString()});

// Player states
for (let s = 0; s < 4; s++) {
  let t = now - (4-s)*7200000;
  for (let st of STATES) {
    events.push({event:'player_state', data:{from:'idle', to:st, duration_ms:Math.floor(Math.random()*300000)+10000, timestamp:new Date(t).toISOString()}, session_id:SESSIONS[s], player_id:PLAYERS[s], timestamp:new Date(t).toISOString()});
    t += Math.floor(Math.random() * 600000) + 30000;
  }
}

// Page views
for (let s = 0; s < 4; s++)
  for (let p of PAGES)
    events.push({event:'page_view', data:{page:p}, session_id:SESSIONS[s], player_id:PLAYERS[s], timestamp:new Date(now - (4-s)*3600000).toISOString()});

// UI clicks
for (let i = 0; i < 30; i++)
  events.push({event:'ui_click', data:{element:pick(CLICKS), timestamp:new Date(now-Math.random()*86400000).toISOString()}, session_id:pick(SESSIONS), player_id:pick(PLAYERS), timestamp:new Date(now-Math.random()*86400000).toISOString()});

// Team saves
for (let i = 0; i < 6; i++) {
  const pkmCount = pick([3,4,5,6]);
  const team = [];
  for (let j = 0; j < pkmCount; j++)
    team.push({speciesID:pick(SPECIES).id, moves:pick(MOVES,4).map(m=>m.id), item:pick(ITEMS), ability:pick(ABILITIES), nature:pick(NATURES)});
  events.push({event:'team_save', data:{team_name:pick(['晴天队','雨天队','沙暴队','龙队','平衡队','空间队','强化队','猛攻队']), pokemon_count:pkmCount, pokemon:team}, session_id:pick(SESSIONS), player_id:pick(PLAYERS), timestamp:new Date(now-Math.random()*86400000).toISOString()});
}

// Battles (detailed)
for (let b = 0; b < 6; b++) {
  const bid = 'battle_' + id();
  const sid = pick(SESSIONS);
  const pid = pick(PLAYERS);
  const side = pick(['a', 'b']);
  const oppType = pick(['human', 'bot']);
  const turns = Math.floor(Math.random() * 12) + 3;
  const base = now - Math.random() * 86400000;
  const myTeamSize = pick([3,4,5,6]);
  const oppTeamSize = pick([3,4,5,6]);
  const teamA = [], teamB = [];
  for (let j = 0; j < myTeamSize; j++) teamA.push({speciesID:pick(SPECIES).id, moves:pick(MOVES,4).map(m=>m.id), item:pick(ITEMS), ability:pick(ABILITIES), nature:pick(NATURES), level:50});
  for (let j = 0; j < oppTeamSize; j++) teamB.push({speciesID:pick(SPECIES).id, moves:pick(MOVES,4).map(m=>m.id), item:pick(ITEMS), ability:pick(ABILITIES), nature:pick(NATURES), level:50});

  // battle_init
  events.push({event:'battle_init', data:{battle_id:bid, side, opponent_type:oppType, side_a:teamA, side_b:teamB}, session_id:sid, player_id:pid, timestamp:new Date(base).toISOString()});

  let myRem = myTeamSize, oppRem = oppTeamSize;
  for (let t = 1; t <= turns; t++) {
    const a1 = pick(['attack','attack','attack','switch']);
    const a2 = pick(['attack','attack','attack','switch']);
    events.push({event:'turn_executed', data:{
      battle_id:bid, turn:t,
      side_a: {type:a1, move_id:a1==='attack'?pick(MOVES).id:null, move_name:a1==='attack'?pick(MOVES).name:null, switch_to:a1==='switch'?pick(SPECIES).id:null},
      side_b: {type:a2, move_id:a2==='attack'?pick(MOVES).id:null, move_name:a2==='attack'?pick(MOVES).name:null, switch_to:a2==='switch'?pick(SPECIES).id:null}
    }, session_id:sid, player_id:pid, timestamp:new Date(base + t*60000).toISOString()});

    const hits = Math.floor(Math.random() * 3) + 1;
    for (let h = 0; h < hits; h++) {
      const dmg = Math.floor(Math.random() * 120) + 5;
      const fnt = Math.random() < 0.2;
      if (fnt) { myRem = Math.max(0, myRem - 1); oppRem = Math.max(0, oppRem - 1); }
      events.push({event:'turn_damage', data:{battle_id:bid, turn:t, target_side:pick(['a','b']), target_species:pick(SPECIES).id, move:pick(MOVES).name, damage:dmg, fainted:fnt}, session_id:sid, player_id:pid, timestamp:new Date(base + t*60000 + 1000).toISOString()});
    }

    if (Math.random() < 0.3) events.push({event:'turn_faint', data:{battle_id:bid, turn:t, species:pick(SPECIES).id, side:pick(['a','b'])}, session_id:sid, player_id:pid, timestamp:new Date(base + t*60000 + 2000).toISOString()});
    if (Math.random() < 0.35) events.push({event:'turn_switch', data:{battle_id:bid, turn:t, species:pick(SPECIES).id, side:pick(['a','b']), reason:pick(['manual','faint_replace'])}, session_id:sid, player_id:pid, timestamp:new Date(base + t*60000 + 3000).toISOString()});
    if (Math.random() < 0.2) events.push({event:'turn_ability', data:{battle_id:bid, turn:t, species:pick(SPECIES).id, side:pick(['a','b']), ability:pick(ABILITY_NAMES)}, session_id:sid, player_id:pid, timestamp:new Date(base + t*60000 + 4000).toISOString()});
    if (Math.random() < 0.15) events.push({event:'turn_heal', data:{battle_id:bid, turn:t, target_side:pick(['a','b']), target_species:pick(SPECIES).id, heal:Math.floor(Math.random()*80)+10}, session_id:sid, player_id:pid, timestamp:new Date(base + t*60000 + 5000).toISOString()});
  }

  const result = myRem > 0 && oppRem === 0 ? 'win' : oppRem > 0 && myRem === 0 ? 'loss' : pick(['win','loss','abandoned']);
  events.push({event:'battle_result', data:{battle_id:bid, side, result, winner: result==='win'?side:(result==='loss'?(side==='a'?'b':'a'):null), turns, own_remaining: myRem, opp_remaining: oppRem}, session_id:sid, player_id:pid, timestamp:new Date(base + turns*60000 + 10000).toISOString()});
}

const out = 'E:/PokemonSImulator/logs/analytics/events_2026-07-06.jsonl';
fs.writeFileSync(out, events.map(e => JSON.stringify(e)).join('\n'));
console.log('Wrote ' + events.length + ' events to ' + out);

const counts = {};
events.forEach(e => { counts[e.event] = (counts[e.event] || 0) + 1 });
console.log('\nEvent breakdown:');
Object.entries(counts).sort((a, b) => b[1] - a[1]).forEach(([k, v]) => console.log('  ' + k + ': ' + v));
