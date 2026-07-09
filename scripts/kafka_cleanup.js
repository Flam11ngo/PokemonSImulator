/**
 * Delete all Kafka records with timestamp before July 7, 2026 00:00:00 UTC.
 * Usage: node scripts/kafka_cleanup.js
 */
const { Kafka } = require('kafkajs');

const BROKER = process.env.KAFKA_BROKER || "100.107.105.99:9092";
const TOPICS = ["battle.logs", "player.ui.events"];
// July 7, 2026 00:00:00 UTC in ms
const JULY7_MS = new Date("2026-07-07T00:00:00Z").getTime();

const kafka = new Kafka({ brokers: [BROKER], connectionTimeout: 10000, requestTimeout: 30000, retry: { retries: 3 } });
const admin = kafka.admin();

async function main() {
  await admin.connect();
  console.log(`Connected to ${BROKER}`);
  console.log(`Target: delete records before ${new Date(JULY7_MS).toISOString()} (${JULY7_MS}ms)`);

  for (const topic of TOPICS) {
    try {
      // Get partition info
      const metadata = await admin.fetchTopicMetadata({ topics: [topic] });
      const partitions = metadata.topics[0]?.particles || metadata.topics[0]?.partitions || [];
      console.log(`\nTopic: ${topic} (${partitions.length} partitions)`);

      // Fetch offsets for July 7 timestamp for each partition
      const offsetsForTimes = partitions.map(p => ({
        topic,
        partition: p.partitionId,
        timestamp: JULY7_MS,
      }));

      const offsetResults = await admin.fetchOffsets({ topics: [topic], timestamp: JULY7_MS });

      // Build delete records request
      const recordsToDelete = [];
      for (const p of partitions) {
        const pid = p.partitionId;
        // Find the offset for this partition
        const result = offsetResults.find(r => r.partition === pid);
        if (result && result.offset > 0) {
          recordsToDelete.push({ topic, partition: pid, offset: result.offset });
          console.log(`  Partition ${pid}: delete before offset ${result.offset}`);
        } else {
          console.log(`  Partition ${pid}: no data to delete`);
        }
      }

      if (recordsToDelete.length > 0) {
        await admin.deleteTopicRecords({ topics: recordsToDelete });
        console.log(`  Done: ${recordsToDelete.length} partitions truncated`);
      }
    } catch (e) {
      console.error(`  Error for topic ${topic}:`, e.message);
    }
  }

  // Also reset consumer groups to latest (they'll only see post-cleanup data)
  try {
    const groups = await admin.listGroups();
    const relevantGroups = groups.filter(g => g.groupId.includes('ads-bridge'));
    for (const g of relevantGroups) {
      console.log(`\nResetting consumer group: ${g.groupId}`);
      // Reset offsets to latest for all topics
      try {
        await admin.resetConsumerGroupOffsets(g.groupId, {
          topic: TOPICS[0],
          offset: 'latest',
        });
        console.log(`  Reset ${g.groupId} → latest`);
      } catch (e) {
        console.log(`  Group ${g.groupId} reset skipped: ${e.message}`);
      }
    }
  } catch (e) {
    console.log('Consumer group reset not available:', e.message);
  }

  await admin.disconnect();
  console.log('\nKafka cleanup complete!');
}

main().catch(e => { console.error(e); process.exit(1); });
