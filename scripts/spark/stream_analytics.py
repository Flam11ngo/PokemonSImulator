#!/usr/bin/env python3
"""
Spark Structured Streaming job: Kafka → aggregate → HDFS.
Runs on Hadoop cluster. Submit with:
  spark-submit --packages org.apache.spark:spark-sql-kafka-0-10_2.12:3.5.0 scripts/spark/stream_analytics.py
"""
from pyspark.sql import SparkSession
from pyspark.sql.functions import from_json, col, window, count, when, sum as _sum
from pyspark.sql.types import StructType, StringType, IntegerType, ArrayType, LongType

KAFKA = "192.168.88.129:9092"
TOPIC = "battle-events"
HDFS_OUT = "/battle_stats"

spark = SparkSession.builder \
    .appName("PokemonBattleAnalytics") \
    .config("spark.sql.streaming.checkpointLocation", "/tmp/spark_checkpoint") \
    .getOrCreate()

schema = StructType() \
    .add("ts", StringType()) \
    .add("type", StringType()) \
    .add("battleId", StringType()) \
    .add("winner", StringType()) \
    .add("turns", IntegerType()) \
    .add("teams", ArrayType(StructType()
        .add("side", StringType())
        .add("speciesId", IntegerType())
        .add("slot", IntegerType())
    ))

df = spark.readStream \
    .format("kafka") \
    .option("kafka.bootstrap.servers", KAFKA) \
    .option("subscribe", TOPIC) \
    .option("startingOffsets", "earliest") \
    .load() \
    .select(from_json(col("value").cast("string"), schema).alias("data")) \
    .select("data.*") \
    .filter(col("type") == "battle_end")

# Species usage and win rate
teams = df.selectExpr("explode(teams) as team").select(
    col("team.speciesId"),
    col("team.side"),
    col("winner"),
    col("ts")
)

# Win rate per species
winrate = teams.groupBy("speciesId").agg(
    _sum(when(col("side") == col("winner"), 1).otherwise(0)).alias("wins"),
    count("*").alias("total")
).withColumn("winrate", col("wins") / col("total"))

# Output to HDFS every 2 minutes
query = winrate.writeStream \
    .outputMode("complete") \
    .format("parquet") \
    .option("path", f"{HDFS_OUT}/species_winrate") \
    .option("checkpointLocation", "/tmp/spark_checkpoint_winrate") \
    .trigger(processingTime="120 seconds") \
    .start()

query.awaitTermination()
