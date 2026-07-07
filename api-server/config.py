"""
PokemonSimulator API Server Configuration

所有环境变量集中管理，支持 Docker 和物理机部署（Linux + Windows）。
大数据组件（Kafka/HDFS/Spark/Redis）为可选，Windows 使用 standalone 模式。
"""
import os
import sys
import platform

# ============================================================
# Platform detection
# ============================================================
IS_WINDOWS = platform.system() == "Windows"
IS_LINUX = platform.system() == "Linux"

# ============================================================
# Server
# ============================================================
API_HOST = os.getenv("API_HOST", "0.0.0.0")
API_PORT = int(os.getenv("API_PORT", "8000"))
DEBUG = os.getenv("DEBUG", "false").lower() in ("true", "1", "yes")

# ============================================================
# Mode: "standalone" (Windows/no-BigData) or "cluster" (Linux+Docker)
# ============================================================
SIMULATOR_MODE = os.getenv("SIMULATOR_MODE", "standalone" if IS_WINDOWS else "cluster")

# ============================================================
# Kafka (外部物理机集群) — 仅 cluster 模式使用
# ============================================================
KAFKA_BROKER = os.getenv("KAFKA_BROKER", "100.107.105.99:9092")

KAFKA_TOPIC_UI_EVENTS = os.getenv("KAFKA_TOPIC_UI_EVENTS", "player.ui.events")
KAFKA_TOPIC_BATTLE_LOG = os.getenv("KAFKA_TOPIC_BATTLE_LOG", "battle.logs")
KAFKA_TOPIC_REQUESTS = os.getenv("KAFKA_TOPIC_REQUESTS", "battle.requests")
KAFKA_TOPIC_RESULTS = os.getenv("KAFKA_TOPIC_RESULTS", "battle.results")
KAFKA_TOPIC_EVENTS = os.getenv("KAFKA_TOPIC_EVENTS", "battle.events")

KAFKA_CONSUMER_GROUP = os.getenv("KAFKA_CONSUMER_GROUP", "api-server-group")

# ============================================================
# PostgreSQL (Docker 或本地)
# ============================================================
POSTGRES_HOST = os.getenv("POSTGRES_HOST", "localhost" if IS_WINDOWS else "postgres")
POSTGRES_PORT = int(os.getenv("POSTGRES_PORT", "5432"))
POSTGRES_USER = os.getenv("POSTGRES_USER", "pokemon")
POSTGRES_PASSWORD = os.getenv("POSTGRES_PASSWORD", "pokemon123")
POSTGRES_DB = os.getenv("POSTGRES_DB", "pokemon_simulator")
DATABASE_URL = os.getenv(
    "DATABASE_URL",
    f"postgresql://{POSTGRES_USER}:{POSTGRES_PASSWORD}@{POSTGRES_HOST}:{POSTGRES_PORT}/{POSTGRES_DB}"
)

# ============================================================
# HDFS (WebHDFS, 外部物理机集群) — 仅 cluster 模式使用
# ============================================================
HDFS_NAMENODE = os.getenv("HDFS_NAMENODE", "100.107.105.99")
HDFS_PORT = int(os.getenv("HDFS_PORT", "9870"))
HDFS_USER = os.getenv("HDFS_USER", "root")
HDFS_BASE_PATH = os.getenv("HDFS_BASE_PATH", "/user/pokemon")

# ============================================================
# Redis (外部物理机集群) — 仅 cluster 模式使用
# ============================================================
REDIS_URL = os.getenv("REDIS_URL", "redis://localhost:6379/0")
REDIS_SENTINEL = os.getenv("REDIS_SENTINEL", "")

# ============================================================
# Spark Master (外部物理机集群) — 仅 cluster 模式使用
# ============================================================
SPARK_MASTER_URL = os.getenv("SPARK_MASTER_URL", "spark://100.107.105.99:7077")
SPARK_REST_URL = os.getenv("SPARK_REST_URL", "http://100.107.105.99:6066")

# ============================================================
# C++ Battle Engine
# ============================================================
if IS_WINDOWS:
    _default_engine = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "build", "PokemonSimulator.exe"
    )
    _default_cache = os.path.join(os.environ.get("TEMP", "C:\\Windows\\Temp"), "pokemon-engine")
else:
    _default_engine = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "build", "PokemonSimulator"
    )
    _default_cache = "/tmp/pokemon-engine"

ENGINE_BINARY = os.getenv("ENGINE_BINARY", _default_engine)
ENGINE_CACHE_DIR = os.getenv("ENGINE_CACHE_DIR", _default_cache)
ENGINE_TIMEOUT = int(os.getenv("ENGINE_TIMEOUT", "30"))  # seconds per turn
