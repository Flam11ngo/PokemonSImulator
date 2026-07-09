"""
Kafka producer for analytics / battle log — fire-and-forget, non-blocking.
"""
import json
import logging
import queue
import threading
from config import KAFKA_BROKER, KAFKA_TOPIC_UI_EVENTS, KAFKA_TOPIC_BATTLE_LOG

logger = logging.getLogger("ws-server")  # use main logger so output is visible

_producer = None
_send_queue = queue.Queue()
_worker_running = False


def _get_producer():
    global _producer
    if _producer is None:
        try:
            from kafka import KafkaProducer
            _producer = KafkaProducer(
                bootstrap_servers=KAFKA_BROKER,
                value_serializer=lambda v: json.dumps(v, ensure_ascii=False).encode('utf-8'),
                key_serializer=lambda k: k.encode('utf-8') if k else None,
                acks=1,
                api_version=(3, 9),
                max_in_flight_requests_per_connection=1,
                retries=3,
            )
            logger.info(f"Kafka producer connected to {KAFKA_BROKER}")
        except Exception as e:
            logger.warning(f"Kafka not available ({e}) — analytics will be file-only")
            _producer = None
    return _producer


def _worker():
    global _worker_running
    logger.info("Kafka worker loop started")
    while _worker_running:
        try:
            topic, key, value = _send_queue.get(timeout=2)
            p = _get_producer()
            if p:
                future = p.send(topic, key=key, value=value)
                future.get(timeout=5)
                logger.info(f"Sent to {topic}: {value.get('event','?')} (queue left: {_send_queue.qsize()})")
            else:
                logger.warning("Kafka producer not available, dropping message")
        except queue.Empty:
            continue
        except Exception as e:
            logger.error(f"Kafka worker error: {e}")


def start():
    global _worker_running
    if _worker_running:
        return
    _worker_running = True
    t = threading.Thread(target=_worker, daemon=True, name="kafka-producer")
    t.start()
    logger.info("Kafka producer worker started")


def send_ui_event(event: dict):
    """Send a player UI analytics event."""
    _send_queue.put((KAFKA_TOPIC_UI_EVENTS, event.get("player_id"), event))
    logger.info(f"Queued ui_event: {event.get('event','?')} queue_size={_send_queue.qsize()}")


def send_battle_log(event: dict):
    """Send a battle log event."""
    key = event.get("data", {}).get("battle_id") or event.get("battle_id", "")
    _send_queue.put((KAFKA_TOPIC_BATTLE_LOG, key, event))
    logger.info(f"Queued battle_log: {event.get('event','?')} key={key} queue_size={_send_queue.qsize()}")
