/**
 * Battle event streaming producer → Kafka.
 * Daemon pushes events to Kafka topic 'battle-events'.
 * Falls back to local .jsonl files if Kafka unavailable.
 */
const fs = require('fs');
const path = require('path');
const WORK_DIR = process.argv[2] || process.cwd();

let kafka = null;
let producer = null;
const TOPIC = 'battle-events';

try {
  const { Kafka } = require('kafkajs');
  kafka = new Kafka({
    clientId: 'pokemon-daemon',
    brokers: (process.env.KAFKA_BROKERS || '192.168.88.129:9092').split(',')
  });
  producer = kafka.producer();
} catch (e) { /* kafkajs not installed, use file fallback */ }

let producerReady = false;
(async () => {
  if (producer) {
    try { await producer.connect(); producerReady = true; } catch (e) { producer = null; }
  }
})();

const FALLBACK_DIR = path.join(process.env.WORK_DIR || WORK_DIR, '..', 'data', 'stream_fallback');
try { fs.mkdirSync(FALLBACK_DIR, { recursive: true }); } catch {}

const TS = () => new Date().toISOString();

async function push(record) {
  const json = JSON.stringify(record);
  // Kafka
  if (producer && producerReady) {
    try {
      await producer.send({ topic: TOPIC, messages: [{ value: json }] });
    } catch (e) { /* fall through to file */ }
  }
  // File fallback
  try {
    fs.appendFileSync(path.join(FALLBACK_DIR, 'events.jsonl'), json + '\n');
  } catch {}
}

module.exports = {
  async pushTurn(battleId, turn, events) {
    await push({
      ts: TS(), type: 'turn', battleId, turn,
      events: events.map(e => ({ type: e.event_type, side: e.side, desc: e.description, value: e.value, move: e.move }))
    });
  },

  async pushBattleEnd(battleId, winner, teams, turns) {
    await push({
      ts: TS(), type: 'battle_end', battleId, winner, turns,
      teams: teams.flat().map(p => ({ side: p.side, speciesId: p.speciesId, slot: p.slot }))
    });
  },

  available() { return !!(producer && producerReady); }
};
