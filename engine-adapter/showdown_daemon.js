/**
 * Showdown battle engine adapter — drop-in replacement for C++ daemon.
 * Uses pokemon-showdown Battle class directly (synchronous, no streams).
 */
const fs = require('fs');
const path = require('path');
const {Battle, Teams, Dex} = require('pokemon-showdown');
// Kafka disabled for local standalone mode
let streamProducer = null;

const WORK_DIR = process.argv[2] || process.cwd();
const IN = p => path.join(WORK_DIR, 'cache', 'input', p);
const OUT = p => path.join(WORK_DIR, 'cache', 'output', p);

function log(msg) { process.stderr.write(`[sd] ${msg}\n`); require('fs').appendFileSync(path.join(WORK_DIR,'daemon.log'), `[sd] ${msg}\n`); }
function rd(p) { try { return JSON.parse(fs.readFileSync(p,'utf-8')); } catch { return null; } }
function wr(p, d) { const t=p+'.tmp'; fs.writeFileSync(t, JSON.stringify(d)); fs.renameSync(t, p); }
function slp(ms) { return new Promise(r => setTimeout(r, ms)); }

// ---- ID → Name resolvers ----
function spName(id) { for (const s of Dex.species.all()) { if (s.num===id) return s.id; } return null; }
function abName(id) { for (const a of Dex.abilities.all()) { if (a.num===id) return a.id; } return null; }
function mvName(id) { for (const m of Dex.moves.all()) { if (m.num===id) return m.id; } return null; }
function itName(id) { for (const i of Dex.items.all()) { if (i.num===id) return i.id; } return null; }
const NATURES = ['Hardy','Lonely','Brave','Adamant','Naughty','Bold','Docile','Relaxed','Impish','Lax','Timid','Hasty','Serious','Jolly','Naive','Modest','Mild','Quiet','Bashful','Rash','Calm','Gentle','Sassy','Careful','Quirky'];

// Reverse: name → ID
function abId(name) { const a = Dex.abilities.get(name); return a?.num||0; }
function itId(name) { const i = Dex.items.get(name); return i?.num||0; }
function mvId(name) { const m = Dex.moves.get(name); return m?.num||0; }
function natId(name) { const i = NATURES.findIndex(n => n.toLowerCase()===String(name).toLowerCase()); return i>=0?i:3; }

// ---- Convert team: our JSON → Showdown set format ----
function convertTeam(list) {
  return list.map(p => {
    const sn = spName(p.speciesID);
    if (!sn) { log(`skip #${p.speciesID}`); return null; }
    const sp = Dex.species.get(sn);
    const an = abName(p.ability) || sp?.abilities?.['0'];
    const mn = (p.moves||[]).map(mid => mvName(mid)).filter(Boolean);
    return {
      name: '', species: sn, item: p.item ? (itName(p.item)||'') : '',
      ability: an||'No Ability',
      moves: mn.length>0 ? mn : ['Tackle'],
      nature: NATURES[p.nature]||'Hardy',
      evs: {hp:0,atk:0,def:0,spa:0,spd:0,spe:0, ...(p.evs||{})},
      ivs: {hp:31,atk:31,def:31,spa:31,spd:31,spe:31},
      level: p.level||50,
    };
  }).filter(Boolean);
}

// ---- Build our JSON state from Showdown Battle object ----
function buildState(battle, st, daemonTurn) {
  return {
    turn: daemonTurn || battle.turn||0,
    battle: {sides: [buildSide(battle.p1,'Player',st), buildSide(battle.p2,'Opponent',st)]},
    events: parseLog(battle, st),
    _weather: weatherJson(battle),
    _field: fieldJson(battle),
  };
}

function buildSide(side, name, st) {
  if (!side) return {name, pokemons:[], active:0, need2switch: false};
  const pkm = (side.pokemon||[]).map((p,i) => {
    if (!p) return {fainted:true, slot:i, speciesId:0, hp:0, maxHp:1};
    const sp = p.species;
    return {
      speciesId: sp?.num||0, _speciesName: sp?.name||'?',
      types: (sp?.types||[]).map(t=>t.toLowerCase()),
      hp: p.hp||0, maxHp: p.maxhp||1, level: p.level||50,
      fainted: p.fainted||p.hp<=0,
      abilityId: abId(p.ability), itemId: itId(p.item),
      nature: natId(p.nature),
      moves: (p.moveSlots||[]).map(ms => {
        const m = ms?.move ? Dex.moves.get(ms.move) : null;
        return { id: mvId(ms?.move||''), pp: ms?.pp||0, maxPp: ms?.maxpp||0, _type: (m?.type||'Normal'), disabled: ms?.disabled || false };
      }),
      _volatiles: p?.volatiles ? Object.keys(p.volatiles) : [],  // debug: show active volatiles
      statStages: [p.boosts?.atk||0,p.boosts?.def||0,p.boosts?.spa||0,p.boosts?.spd||0,p.boosts?.spe||0,p.boosts?.accuracy||0,p.boosts?.evasion||0],
      inBattleStatus: p.status ? [{id:{brn:1,frz:2,par:3,psn:4,slp:5,tox:7}[p.status]||0,name:p.status}] : [],
      _charging: p?.volatiles?.twoturnmove?.move ? mvId(p.volatiles.twoturnmove.move) : null,
      slot: i,
    };
  });
  // Initialize _lastHp for accurate first-damage calc
  const sid = name === 'Player' ? 'p1' : 'p2';
  pkm.forEach((p,i) => { if (p && !p.fainted && p.hp > 0 && st._lastHp[sid] == null) st._lastHp[sid] = p.hp; });
  const activeIdx = side.active?.[0]?.position ?? pkm.findIndex(p=>!p.fainted);
  const activePokemon = pkm[activeIdx];
  const hasBench = pkm.some((p,i) => !p.fainted && i !== activeIdx);
  const forceSwitch = side?.activeRequest?.forceSwitch?.[0] === true;
  const needSwitch = (activePokemon?.fainted || forceSwitch) && hasBench;
  if (forceSwitch) log(`[need2switch] ${name}: forceSwitch=true (activeRequest detected)`);
  return {name:side.name||name, pokemons:pkm, active:activeIdx>=0?activeIdx:0, need2switch: needSwitch, sideEffects:{}};
}

function weatherJson(b) {
  const f = b?.field;
  const w = (f?.weather || '').toLowerCase();
  const ws = f?.weatherState || {};
  const left = ws?.duration ?? 0;
  const map = {
    raindance: {type:1, label:'雨天☔'}, rain: {type:1, label:'雨天☔'},
    sunnyday: {type:2, label:'晴天☀️'}, sun: {type:2, label:'晴天☀️'},
    sandstorm: {type:3, label:'沙暴🏜️'},
    hail: {type:4, label:'冰雹🌨️'},
    snow: {type:5, label:'雪天❄️'},
    desolateland: {type:2, label:'大日照☀️'},
    primordialsea: {type:1, label:'大雨🌧️'},
    deltastream: {type:6, label:'乱流🌪️'},
  };
  const m = map[w] || {type:0, label:''};
  return {type: m.type, label: m.label, duration: left};
}
function fieldJson(b) {
  const f = b?.field;
  const p=[];
  if (f?.isGravity) p.push('重力');
  if (f?.isTrickRoom) p.push('戏法空间🔄');
  if (f?.isMagicRoom) p.push('魔法空间');
  if (f?.isWonderRoom) p.push('奇妙空间');
  // Terrain: Showdown stores terrain ID string in f.terrain, duration in f.terrainState.duration
  const t = (f?.terrain || '').toLowerCase();
  const terrainNames = {electricterrain:'电气场地⚡', psychicterrain:'精神场地🔮', grassyterrain:'青草场地🌿', mistyterrain:'薄雾场地🌫️'};
  if (terrainNames[t]) p.push(terrainNames[t]);
  const dur = f?.terrainState?.duration ?? 0;
  return {type:p.length?1:0, label:p.join(','), _terrain: t, duration: dur};
}

// Per-battle mutable state (no globals — safe for concurrent battles)
function createBattleState() {
  return { logIndex: 0, _lastHp: {}, _pendingEff: '', _pendingMove: null, _lastSwitch: {} };
}

function pokemonSide(pokename) {
  // Map Showdown p1/p2 to frontend a/b: p1=Player=sideA, p2=Opponent=sideB
  if (!pokename) return '';
  const colon = pokename.indexOf(':');
  const raw = colon >= 0 ? pokename.substring(0, 2) : '';  // "p1" or "p2"
  return raw === 'p1' ? 'a' : raw === 'p2' ? 'b' : '';
}

function parseLog(battle, st) {
  if (!battle?.log) return [];
  const events = [];
  const seenKeys = new Set();
  while (st.logIndex < battle.log.length) {
    const line = battle.log[st.logIndex++];
    if (!line.startsWith('|')) continue;
    const parts = line.split('|').filter(Boolean);
    if (parts.length<2) continue;
    const kw = parts[0];
    // Protocol: |switch| (intentional) and |drag| (forced) are alternatives for the
    // same action. Keep only |switch| — it carries the full state (DETAILS/HP/STATUS).
    if (['t:','upkeep','turn','split','clearpoke','teampreview','player','teamsize','gametype','gen','tier','poke','start','done','prematureend','','request','drag'].includes(kw)) continue;
    const ev = formatEvent(kw, parts, st);
    if (ev) {
      // Each protocol line maps to at most one event.  No synthetic duplicates.
      const key = kw + '_' + st.logIndex;
      if (seenKeys.has(key)) continue;
      seenKeys.add(key);
      events.push({ ...ev, turn_index: battle.turn||0, _key: battle.turn + '_' + st.logIndex });
    }
  }
  return events;
}

function formatEvent(kw, parts, st) {
  const rest = parts.slice(1);
  const base = { side: pokemonSide(rest[0]), event_type: classifyEv(kw) };

  // ---- WEATHER ----
  if (kw === '-weather') {
    const w = rest[0] || '';
    return { ...base, event_type:'weather', description: w.replace(' [upkeep]','') };
  }

  // ---- MOVE (store for merging into next damage/heal/miss) ----
  if (kw === 'move') {
    st._pendingMove = { user: pokemonLabel(rest[0]), move: rest[1], side: pokemonSide(rest[0]) };
    return null;  // merged into next actionable event
  }

  // ---- SWITCH ----
  if (kw === 'switch' || kw === 'drag' || kw === 'replace') {
    const side = pokemonSide(rest[0]);
    const name = rest[1].split(',')[0];
    const switchFingerprint = side + '|' + name;
    // Showdown may emit duplicate |switch| lines for the same state sync —
    // only emit the first one (state-driven idempotency, matching Showdown client behavior)
    if (st._lastSwitch[side] === switchFingerprint) return null;
    st._lastSwitch[side] = switchFingerprint;
    st._lastHp[side] = null;
    const who = pokemonLabel(rest[0]);
    return { ...base, side, event_type:'switch_in', description: `${who} 换上 ${name}` };
  }

  // ---- FAINT (Major Action per Showdown protocol — independent of -damage) ----
  if (kw === 'faint') {
    return { ...base, side: pokemonSide(rest[0]), event_type:'faint', description: `${pokemonLabel(rest[0])} 倒下 💀` };
  }

  // ---- DAMAGE (Minor Action — merge move + effectiveness + crit for UX) ----
  if (kw === '-damage') {
    const raw = rest[1]||'';
    const fnt = raw.includes('fnt');
    const hpParts = raw.split(' ')[0].split('/');
    const remaining = parseInt(hpParts[0]) || 0;
    const total = parseInt(hpParts[1]) || 0;
    const sideKey = pokemonSide(rest[0]);
    const prevHp = st._lastHp[sideKey] ?? total ?? (remaining > 0 ? remaining : 0);
    const dmg = prevHp - remaining;
    if (fnt && dmg <= 0 && prevHp === 0) { st._pendingEff = ''; st._pendingMove = null; return null; }
    st._lastHp[sideKey] = remaining;
    if (dmg <= 0 && !fnt) { st._pendingEff = ''; st._pendingMove = null; return null; }

    // Determine damage type: attack, status (brn/psn), weather, etc.
    const from = (rest[2] || '').replace(/^\[from\]\s*/, '');
    const isDot = /^(brn|psn|tox|slp|confusion|sandstorm|hail|snow|spikes|stealthrock|leechseed|curse|nightmare|bind|wrap|fire spin|whirlpool|clamp|infestation|sandtomb|snaptrap|thunder cage|gmaxwildfire|gmaxvolcalith|gmaxcentiferno|gmaxsandblast|saltcure|trapped)$/i.test(from);
    const isAttack = !!st._pendingMove || !!st._pendingEff;  // preceded by move or effectiveness

    const eff = st._pendingEff; st._pendingEff = '';
    const mv = st._pendingMove; st._pendingMove = null;

    let desc = '';
    if (isDot) {
      const names = { brn:'🔥灼伤', psn:'☠️中毒', tox:'☠️剧毒', slp:'💤梦话伤害', sandstorm:'🏜️沙暴', hail:'🌨️冰雹', snow:'❄️雪天', leechseed:'🌱寄生种子', confusion:'💫混乱', curse:'👻诅咒', nightmare:'😈噩梦', saltcure:'🧂腌盐', spikes:'📍撒菱', stealthrock:'🪨隐形岩', bind:'🪢紧束', wrap:'🪢缠绕', 'fire spin':'🔥火焰旋涡', whirlpool:'🌊潮旋', clamp:'🦪贝壳夹', infestation:'🐛死缠烂打', sandtomb:'🏜️流沙地狱', snaptrap:'🪤捕兽夹', 'thunder cage':'⚡雷电囚笼', trapped:'⛓️束缚' };
      desc = `${names[from.toLowerCase()] || from} `;
    } else if (mv) {
      desc = `${mv.user} 使出 ${mv.move}！`;
    } else {
      desc = '受到攻击 ';  // fallback when no move context available
    }
    const val = isNaN(dmg) ? 0 : dmg;
    return { ...base, side: sideKey, event_type: isDot ? 'dot' : 'damage', description: desc + eff + `-${val} HP` + (fnt ? ' 💀' : ''), value: val };
  }
  // Store effectiveness hint for next damage event
  if (kw === '-supereffective') { st._pendingEff = '💥 效果拔群 '; return null; }
  if (kw === '-resisted') { st._pendingEff = '🛡️ 效果不理想 '; return null; }
  // ---- CRIT ----
  if (kw === '-crit') { st._pendingEff = (st._pendingEff||'') + '💢 会心一击 '; return null; }

  // ---- HEAL (merge move) ----
  if (kw === '-heal') {
    const hpParts = (rest[1]||'').split('/');
    const remaining = parseInt(hpParts[0]) || 0;
    const total = parseInt(hpParts[1]) || 0;
    const sideKey = pokemonSide(rest[0]);
    const prevHp = st._lastHp[sideKey] ?? total;
    const heal = remaining - prevHp;
    st._lastHp[sideKey] = remaining;
    if (heal <= 0) return null;
    const mv = st._pendingMove; st._pendingMove = null;
    const desc = mv ? `${mv.user} 使出 ${mv.move}！` : '';
    return { ...base, side: sideKey, event_type:'heal', description: desc + `+${heal} HP`, value: heal };
  }

  // ---- STATUS ----
  if (kw === '-status') {
    const statusNames = {brn:'烧伤🔥',frz:'冰冻❄️',par:'麻痹⚡',psn:'中毒☠️',tox:'剧毒☠️',slp:'睡眠💤'};
    const s = statusNames[rest[1]] || rest[1];
    const mv = st._pendingMove; st._pendingMove = null; st._pendingEff = '';
    const prefix = mv ? `${mv.user} 使出 ${mv.move}！` : '';
    return { ...base, side: pokemonSide(rest[0]), event_type:'status_apply', description: prefix + s };
  }
  if (kw === '-curestatus') {
    const mv = st._pendingMove; st._pendingMove = null; st._pendingEff = '';
    const prefix = mv ? `${mv.user} 使出 ${mv.move}！` : '';
    return { ...base, side: pokemonSide(rest[0]), event_type:'heal', description: prefix + '状态治愈 ✨' };
  }

  // ---- STAT CHANGE ----
  if (kw === '-boost') {
    const st = STAT_MAP[rest[1]] || rest[1];
    const amt = parseInt(rest[2]) || 1;
    const mv = st._pendingMove; st._pendingMove = null; st._pendingEff = '';
    const prefix = mv ? `${mv.user} 使出 ${mv.move}！` : '';
    return { ...base, side: pokemonSide(rest[0]), event_type:'stat_raise', description: prefix + `${st} ▲`, stat: st, value: amt };
  }
  if (kw === '-unboost') {
    const st = STAT_MAP[rest[1]] || rest[1];
    const mv = st._pendingMove; st._pendingMove = null; st._pendingEff = '';
    const prefix = mv ? `${mv.user} 使出 ${mv.move}！` : '';
    return { ...base, side: pokemonSide(rest[0]), event_type:'stat_drop', description: prefix + `${st} ▼` };
  }

  // ---- ABILITY ----
  if (kw === '-ability') {
    const mv = st._pendingMove; st._pendingMove = null; st._pendingEff = '';
    const prefix = mv ? `${mv.user} 使出 ${mv.move}！` : '';
    return { ...base, side: pokemonSide(rest[0]), event_type:'ability_trigger', description: prefix + `${rest[1]||'特性'} 发动` };
  }
  if (kw === '-activate') return { ...base, side: pokemonSide(rest[0]), event_type:'ability_trigger', description: rest[1] || '效果发动' };

  // ---- ITEM ----
  if (kw === '-item' || kw === '-enditem') return { ...base, event_type:'item_trigger', side: pokemonSide(rest[0]), description: '道具发动' };

  // ---- MISS / IMMUNE / FAIL / BLOCK (merge move) ----
  if (kw === '-miss' || kw === '-immune' || kw === '-fail' || kw === '-block') {
    const who = pokemonLabel(rest[0]);
    const mv = st._pendingMove; st._pendingMove = null; st._pendingEff = '';
    const desc = mv ? `${mv.user} 使出 ${mv.move}！` : '';
    const msg = kw === '-miss' ? `${who} 的攻击未命中`
      : kw === '-immune' ? `${who} 不受影响`
      : kw === '-fail' ? `${who} 失败了`
      : `${who} 保护了自己`;
    return { ...base, side: pokemonSide(rest[0]), event_type:'info', description: desc + msg };
  }

  // ---- VOLATILE START / END ----
  if (kw === '-start') {
    const who = pokemonLabel(rest[0]);
    const effect = rest[1] || '';
    const names = {confusion:'混乱😵',leechseed:'寄生种子🌱',curse:'诅咒',perishsong:'灭亡之歌🎵',taunt:'挑衅',torment:'折磨',healblock:'回复封锁',embargo:'查封',infatuation:'着迷💕',nightmare:'噩梦',yawn:'哈欠💤',substitute:'替身',protect:'保护🛡️',endure:'忍耐',helpinghand:'帮助🤝',focusenergy:'集气',destinybond:'同命🔮',encore:'再来一次',disable:'定身法',ingrain:'扎根🌿',aquaring:'水流环💧',magnetrise:'电磁悬浮🧲',telekinesis:'意念移物🔮',powertrick:'力量戏法',laserfocus:'激光聚焦',foresight:'识破👁️',miracleeye:'奇迹之眼👁️',imprison:'封印🔒',grudge:'怨念😈',snatch:'抢夺🤏',magiccoat:'魔法外衣🪄',electrify:'等离子浴⚡'};
    const label = names[effect] || effect;
    return { ...base, side: pokemonSide(rest[0]), event_type:'status_apply', description: `${who} 陷入了 ${label}` };
  }
  if (kw === '-end') {
    const who = pokemonLabel(rest[0]);
    const effect = rest[1] || '';
    return { ...base, side: pokemonSide(rest[0]), event_type:'info', description: `${who} 的 ${effect} 解除了` };
  }

  // ---- FIELD / SIDE CONDITIONS ----
  if (kw === '-fieldstart') {
    const effect = rest[0] || '';
    const names = {electricterrain:'电气场地⚡',psychicterrain:'精神场地🔮',grassyterrain:'青草场地🌿',mistyterrain:'薄雾场地🌫️',gravity:'重力🌍',trickroom:'戏法空间🔄',magicroom:'魔法空间🪄',wonderroom:'奇妙空间❓',waterpledge:'水之誓约🌊',firepledge:'火之誓约🔥',grasspledge:'草之誓约🌿'};
    return { ...base, event_type:'weather', description: names[effect] || effect };
  }
  if (kw === '-fieldend') {
    const effect = rest[0] || '';
    return { ...base, event_type:'info', description: `${effect} 结束了` };
  }
  if (kw === '-sidestart') return { ...base, event_type:'info', description: rest[0] };
  if (kw === '-sideend') {
    const effect = rest[1] || rest[0] || '';
    return { ...base, event_type:'info', description: `${effect} 效果结束` };
  }

  // ---- MESSAGE / DETAILS ----
  if (kw === '-message') return { ...base, event_type:'info', description: rest.join(' ') };
  if (kw === 'detailschange') {
    // Form change: "|detailschange|p1a: Zacian-Crowned|"
    const colon = (rest[0] || '').indexOf(':');
    const newName = colon >= 0 ? rest[0].slice(colon + 1).trim() : (rest[0] || '');
    return { ...base, event_type:'detailschange', description: `形态变化 → ${newName}`, _newSpeciesName: newName };
  }

  // Skip raw HP/status numbers (e.g. "p1a: Pikachu 67/110")
  return null;
}

const STAT_MAP = {atk:'物攻',def:'物防',spa:'特攻',spd:'特防',spe:'速度',accuracy:'命中',evasion:'闪避'};

function pokemonLabel(pokename) {
  // pokename format: "p1a: Charizard" or "p2a: Venusaur"
  if (!pokename) return '';
  const colon = pokename.indexOf(':');
  const name = colon >= 0 ? pokename.slice(colon + 1).trim() : pokename;
  const id = pokename.substring(0, 2); // p1 or p2
  const side = id === 'p1' ? '己方' : id === 'p2' ? '对方' : '';
  return `${side} ${name}`.trim();
}

function classifyEv(k) {
  if (k==='move') return 'info';            // move declaration (just message)
  if (k==='-damage'||k==='-supereffective'||k==='-resisted') return 'damage';
  if (k==='-heal') return 'heal';
  if (k==='-status'||k==='-curestatus') return 'status_apply';
  if (k==='-boost') return 'stat_raise';
  if (k==='-unboost') return 'stat_drop';
  if (k==='-ability'||k==='-activate') return 'ability_trigger';
  if (k==='-item'||k==='-enditem') return 'item_trigger';
  if (k==='switch'||k==='drag'||k==='replace') return 'switch_in';
  if (k==='faint') return 'faint';
  if (k==='-weather') return 'weather';
  return 'info';
}

function convAction(a, side, battle) {
  const t = a.type||'pass';
  if (t==='attack'||t==='move') {
    return `move ${(a.move_index||0)+1}`;
  }
  if (t==='switch') return `switch ${(a.switch_index||0)+1}`;
  return 'pass';
}

// ---- Data dump mode: export all Dex data as JSON (replaces PokeAPI-based SQLite) ----
if (process.argv[2] === '--dump-data') {
  const {Dex: D} = require('pokemon-showdown');
  const outFile = process.argv[3] || path.join(__dirname, '..', 'data', 'showdown_data.json');
  const log = msg => process.stderr.write(`[sd-dump] ${msg}\n`);
  const STAT_KEYS = ['hp','atk','def','spa','spd','spe'];
  const species = D.species.all().map(s => ({
    id: s.num, name: s.name, types: s.types||[],
    baseSpecies: s.baseSpecies||null,
    baseStats: STAT_KEYS.map(k => (s.baseStats||{})[k]||0),
    abilities: Object.values(s.abilities||{}).filter(a=>typeof a==='number').map(a=>a),
    hiddenAbility: Object.values(s.abilities||{}).findIndex((a,i)=>a==='H') >= 0 ? Object.values(s.abilities||{}).filter(a=>typeof a==='number').pop() : 0
  }));
  const moves = D.moves.all().map(m => ({
    id: m.num, name: m.name, type: m.type, category: m.category,
    power: m.basePower||0, accuracy: m.accuracy===true ? 101 : (m.accuracy||0), pp: m.pp||0, description: (m.shortDesc||'').slice(0,80)
  }));
  const abilities = D.abilities.all().map(a => ({
    id: a.num, name: a.name, description: (a.shortDesc||'').slice(0,80)
  }));
  const items = D.items.all().map(i => ({
    id: i.num, name: i.name, description: (i.shortDesc||'').slice(0,80)
  }));
  // Learnsets: Dex.data.Learnsets keyed by Showdown species ID (e.g. "bulbasaur")
  // IMPORTANT: Merge forms that share the same NatDex number (e.g. zacian + zaciancrowned)
  const speciesIdToNum = {};
  for (const s of D.species.all()) speciesIdToNum[s.id] = s.num;
  const learnsets = {};
  for (const [sdId, entry] of Object.entries(D.data.Learnsets||{})) {
    const snum = speciesIdToNum[sdId];
    if (!snum || !entry?.learnset) continue;
    const ids = [];
    for (const mid of Object.keys(entry.learnset)) {
      const mv = D.moves.get(mid); if (mv?.num) ids.push(mv.num);
    }
    if (learnsets[snum]) {
      learnsets[snum].push(...ids);
    } else {
      learnsets[snum] = ids;
    }
  }
  // Deduplicate after merging all forms
  for (const snum of Object.keys(learnsets)) {
    learnsets[snum] = [...new Set(learnsets[snum])];
  }
  const speciesAbilities = {};
  for (const s of D.species.all()) {
    const abs = [];
    try {
      for (const [k,v] of Object.entries(s.abilities||{})) {
        const a = D.abilities.get(v); if (a?.num) abs.push({ speciesId: s.num, abilityId: a.num, isHidden: k==='H'||k==='S' });
      }
    } catch {}
    if (abs.length) speciesAbilities[s.num] = abs;
  }
  const dump = { species, moves, abilities, items, learnsets, speciesAbilities };
  fs.mkdirSync(path.dirname(outFile), { recursive: true });
  fs.writeFileSync(outFile, JSON.stringify(dump));
  log(`Dumped ${species.length} species, ${moves.length} moves, ${abilities.length} abilities, ${items.length} items → ${outFile}`);
  process.exit(0);
}

// ============================================================
async function main() {
  log(`start, work: ${WORK_DIR}`);

  // Per-battle state (no globals)
  const st = createBattleState();

  // Persistent logging directory for analytics
  const TS = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
  const LOG_DIR = path.join(WORK_DIR, '..', 'data', 'logs', TS.slice(0, 10), TS.slice(11) + '_' + Date.now().toString(36));
  function logPersistent(subdir, filename, data) {
    try {
      const dir = path.join(LOG_DIR, subdir);
      fs.mkdirSync(dir, { recursive: true });
      fs.writeFileSync(path.join(dir, filename), JSON.stringify(data, null, 2));
    } catch(e) { log(`logPersistent error: ${e.message}`); }
  }

  // Wait for init files
  let sa, sb;
  log('waiting for side_a.json + side_b.json...');
  while (true) { sa=rd(IN('side_a.json')); sb=rd(IN('side_b.json')); if (sa&&sb) break; await slp(300); }
  log('init loaded');

  // Persist input teams
  logPersistent('input', '0000_side_a.json', sa);
  logPersistent('input', '0000_side_b.json', sb);

  const ta = convertTeam(sa.pokemon||[]), tb = convertTeam(sb.pokemon||[]);
  if (!ta.length||!tb.length) { log('ERROR: empty team'); process.exit(1); }
  log(`teams: ${ta.length} vs ${tb.length}`);

  const battle = new Battle({formatid:'gen9customgame',seed:[42,1,3,7],debug:true});
  battle.join('p1', 'Player', 'Player', Teams.pack(ta));
  battle.join('p2', 'Opponent', 'Opponent', Teams.pack(tb));
  // Team preview
  battle.choose('p1', 'team 1');
  battle.choose('p2', 'team 1');
  log(`battle start, turn ${battle.turn}, p1=${battle.p1.active[0]?.species?.name}, p2=${battle.p2.active[0]?.species?.name}`);

  // Skip setup log lines (don't show as events)
  st.logIndex = battle.log.length;

  // Turn 0 output
  const s0 = buildState(battle, st, 0);
  wr(OUT('output_0.json'), s0);
  logPersistent('output', '0000_turn.json', s0);
  log('output_0.json written');

  // Main turn loop
  let tn = 0;
  while (!battle.ended) {
    tn++;
    log(`waiting turn ${tn}...`);
    let a1=null, a2=null, tw=0;
    const TURN_TIMEOUT = 300; // 60 seconds for human players
    const BOT_TIMEOUT = 3; // 3 iterations (600ms) then auto-generate bot action
    while (!a1||!a2) {
      if (!a1) { const d=rd(IN(`1_input_${tn}.json`)); if(d) a1=convAction(d,'p1',battle); }
      if (!a2) { const d=rd(IN(`2_input_${tn}.json`)); if(d) a2=convAction(d,'p2',battle); }
      if (battle.ended) break;
      if (++tw > TURN_TIMEOUT) { log(`TURN TIMEOUT waiting turn ${tn}: a1=${!!a1} a2=${!!a2}`); process.exit(1); }
      // Auto-bot: if p2 (NPC) has no input after BOT_TIMEOUT, use random move
      if (!a2 && tw >= BOT_TIMEOUT) {
        const p2moves = battle.p2?.active[0]?.moveSlots || [];
        const validMoves = p2moves.filter(m => m?.pp > 0 && !m?.disabled);
        const randMove = validMoves.length > 0
          ? `move ${validMoves[Math.floor(Math.random() * validMoves.length)].id}`
          : 'move 1';
        a2 = randMove;
        log(`auto-bot: p2 ${randMove}`);
      }
      await slp(200);
    }
    if (battle.ended) break;

    // Persist input actions
    logPersistent('input', `${tn.toString().padStart(4,'0')}_1_input.json`, rd(IN(`1_input_${tn}.json`))||{});
    logPersistent('input', `${tn.toString().padStart(4,'0')}_2_input.json`, rd(IN(`2_input_${tn}.json`))||{});

    log(`turn ${tn}: p1=${a1} p2=${a2} | p1.active=${battle.p1?.active[0]?.species?.name||'?'} p2.active=${battle.p2?.active[0]?.species?.name||'?'}`);
    let r1 = battle.choose('p1', a1);
    if (!r1) {
      log(`[choose-err] p1 rejected: "${battle.p1?.choice?.error||'no error msg'}" — auto-fallback`);
      // Auto-pick a random valid move for the human when their choice is rejected
      const p1moves = battle.p1?.active[0]?.moveSlots || [];
      const validMoves = p1moves.filter(m => m?.pp > 0 && !m?.disabled);
      if (validMoves.length > 0) {
        const fallback = `move ${validMoves[Math.floor(Math.random() * validMoves.length)].id}`;
        log(`[choose-err] p1 fallback: ${fallback}`);
        r1 = battle.choose('p1', fallback);
      } else {
        // All moves disabled — pass
        log(`[choose-err] p1 no valid moves, passing`);
        r1 = battle.choose('p1', 'pass');
      }
    }
    if (!r1) log(`[choose-err] p1 STILL rejected after fallback: "${battle.p1?.choice?.error||'no error msg'}"`);
    const r2 = battle.choose('p2', a2);
    if (!r2) log(`[choose-err] p2 rejected: "${battle.p2?.choice?.error||'no error msg'}" requestState=${battle.requestState}`);
    log(`[debug] choose results: p1=${r1} p2=${r2} requestState=${battle.requestState} midTurn=${battle.midTurn} ended=${battle.ended} turn=${battle.turn} logLen=${battle.log?.length||0}`);
    // Let Showdown settle (U-turn etc. sets activeRequest.forceSwitch after choose)
    await slp(50);

    let state = buildState(battle, st, tn);
    // Debug: log move disabled status for active Pokemon
    const logSide = (side, label) => {
      const s = side?.pokemons?.[side?.active||0];
      if (!s) return `${label}: ?`;
      const mvInfo = (s.moves||[]).map(m => `#${m.id} pp${m.pp}/${m.maxPp} disabled=${m.disabled||'-'}`).join(', ');
      return `${label} ${s._speciesName} hp=${s.hp}/${s.maxHp} volatiles=[${(s._volatiles||[]).join(',')}] moves: ${mvInfo}`;
    };
    log(`[state] ${logSide(state.battle.sides[0], 'p1')} | ${logSide(state.battle.sides[1], 'p2')}`);
    wr(OUT(`output_${tn}.json`), state);
    logPersistent('output', `${tn.toString().padStart(4,'0')}_turn.json`, state);
    log(`output_${tn}.json done (daemonTurn=${tn} engineTurn=${battle.turn}, events=${state.events?.length||0}, need2switch: p1=${state.battle.sides[0]?.need2switch} p2=${state.battle.sides[1]?.need2switch}, requestState=${battle.requestState}, midTurn=${battle.midTurn}, ended=${battle.ended})`);

    // Handle forced switches (Pokemon fainted)
    while (!battle.ended) {
      state = buildState(battle, st, tn);
      const s1 = state.battle.sides[0];
      const s2 = state.battle.sides[1];
      const needP1 = s1.need2switch;
      const needP2 = s2.need2switch;
      if (!needP1 && !needP2) break;

      log(`force switch needed: p1=${needP1} p2=${needP2}`);
      let fs1=null, fs2=null;
      let fsw = 0;
      const MAX_WAIT = 150;
      while ((needP1 && !fs1) || (needP2 && !fs2)) {
        if (needP1 && !fs1) { const d=rd(IN(`1_input_${tn}_force.json`)); if(d) fs1=convAction(d,'p1',battle); }
        if (needP2 && !fs2) { const d=rd(IN(`2_input_${tn}_force.json`)); if(d) fs2=convAction(d,'p2',battle); }
        if (++fsw > MAX_WAIT) { log(`FORCE SWITCH TIMEOUT after ${MAX_WAIT*200}ms, breaking`); break; }
        await slp(200);
      }

      if (needP1) { battle.choose('p1', fs1); log(`p1 force switch: ${fs1}`); }
      if (needP2) { battle.choose('p2', fs2); log(`p2 force switch: ${fs2}`); }

      logPersistent('input', `${tn.toString().padStart(4,'0')}_1_input_force.json`, rd(IN(`1_input_${tn}_force.json`))||{});
      logPersistent('input', `${tn.toString().padStart(4,'0')}_2_input_force.json`, rd(IN(`2_input_${tn}_force.json`))||{});

      state = buildState(battle, st, tn);
      // Debug: log move disabled after force switch
      const logFsSide = (side, label) => {
        const s = side?.pokemons?.[side?.active||0];
        if (!s) return `${label}: ?`;
        const mvInfo = (s.moves||[]).map(m => `#${m.id} pp${m.pp}/${m.maxPp} disabled=${m.disabled||'-'}`).join(', ');
        return `${label} ${s._speciesName} hp=${s.hp}/${s.maxHp} volatiles=[${(s._volatiles||[]).join(',')}] moves: ${mvInfo}`;
      };
      log(`[state-fs] ${logFsSide(state.battle.sides[0], 'p1')} | ${logFsSide(state.battle.sides[1], 'p2')}`);
      wr(OUT(`output_${tn}_force.json`), state);
      logPersistent('output', `${tn.toString().padStart(4,'0')}_force.json`, state);
      log(`output_${tn}_force.json done`);
    }
  }

  const finalState = buildState(battle, st, battle.turn||0);
  wr(OUT('output_final.json'), finalState);
  logPersistent('output', 'final.json', finalState);

  // Battle summary for analytics
  // Push to streaming pipeline
  if (streamProducer) {
    const winner = battle.winner === 'Player' ? 'a' : battle.winner === 'Opponent' ? 'b' : '';
    streamProducer.pushBattleEnd({
      battle_id: path.basename(WORK_DIR),
      winner,
      turns: battle.turn,
      teams: [
        ta.map((p,i) => ({side:'a',speciesId:Dex.species.get(p.species)?.num||0,slot:i})),
        tb.map((p,i) => ({side:'b',speciesId:Dex.species.get(p.species)?.num||0,slot:i}))
      ]
    });
  }

  logPersistent('.', 'battle_summary.json', {
    battle_id: LOG_DIR.split('/').pop() || Date.now().toString(36),
    teams: {
      a: (sa.pokemon||[]).map(p => ({ id: p.speciesID, _name: spName(p.speciesID) })),
      b: (sb.pokemon||[]).map(p => ({ id: p.speciesID, _name: spName(p.speciesID) })),
    },
    turns: battle.turn,
    winner: battle.winner,
    ended: battle.ended,
    timestamp: new Date().toISOString(),
  });
  log(`battle ended, logs: ${LOG_DIR}`);
}

main().catch(e => { log(`FATAL: ${e.stack}`); process.exit(1); });
