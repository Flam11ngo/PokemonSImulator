"""
Windows compatibility shim for services that require big-data infrastructure.

When SIMULATOR_MODE=standalone (Windows default), all big-data services
(Kafka, HDFS, Redis, Spark) return safe no-op stubs so the API server
can start without crash. The standalone_server.py WebSocket gateway is
the primary Windows entrypoint and doesn't need these at all.
"""
import os
import logging

logger = logging.getLogger("services.compat")

MODE = os.getenv("SIMULATOR_MODE", "standalone")
IS_STANDALONE = MODE == "standalone"

if IS_STANDALONE:
    logger.info("Running in standalone mode — Kafka/HDFS/Redis/Spark stubs active")
