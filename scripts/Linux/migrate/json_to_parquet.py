#!/usr/bin/env python3
"""
JSON → HDFS Parquet ETL 迁移脚本 (Person B)

将 data/*.json 静态数据清洗并转换为 Parquet 格式，上传到 HDFS。

用法:
    python3 json_to_parquet.py [--local] [--hdfs-namenode hdfs://node1:8020]

输出:
    --local:  写入本地 ./output/ 目录
    --hdfs:   写入 HDFS /user/pokemon/static/

依赖:
    pip install pyspark
"""
import argparse
import json
import os
import sys
from datetime import datetime

# PySpark
from pyspark.sql import SparkSession
from pyspark.sql.types import (
    StructType, StructField, StringType, IntegerType, BooleanType,
    ArrayType, FloatType
)

# ============================================================
# Schema 定义
# ============================================================

SPECIES_SCHEMA = StructType([
    StructField("id", IntegerType(), False),
    StructField("name", StringType(), False),
    StructField("type1", StringType(), True),
    StructField("type2", StringType(), True),
    StructField("base_hp", IntegerType(), True),
    StructField("base_atk", IntegerType(), True),
    StructField("base_def", IntegerType(), True),
    StructField("base_spa", IntegerType(), True),
    StructField("base_spd", IntegerType(), True),
    StructField("base_spe", IntegerType(), True),
    StructField("height", FloatType(), True),
    StructField("weight", FloatType(), True),
    StructField("abilities", ArrayType(IntegerType()), True),
    StructField("hidden_ability_id", IntegerType(), True),
    StructField("learnable_moves", ArrayType(IntegerType()), True),
    StructField("egg_group1", StringType(), True),
    StructField("egg_group2", StringType(), True),
    StructField("evolution_level", IntegerType(), True),
    StructField("next_evolution_id", IntegerType(), True),
    StructField("male_ratio", FloatType(), True),
])

MOVES_SCHEMA = StructType([
    StructField("id", IntegerType(), False),
    StructField("name", StringType(), False),
    StructField("type", StringType(), True),
    StructField("category", StringType(), True),
    StructField("power", IntegerType(), True),
    StructField("accuracy", IntegerType(), True),
    StructField("pp", IntegerType(), True),
    StructField("priority", IntegerType(), True),
    StructField("target", StringType(), True),
    StructField("effect", StringType(), True),
    StructField("effect_chance", IntegerType(), True),
    StructField("description", StringType(), True),
])

ABILITIES_SCHEMA = StructType([
    StructField("id", IntegerType(), False),
    StructField("name", StringType(), False),
    StructField("api_name", StringType(), True),
    StructField("description", StringType(), True),
    StructField("mapped_type", StringType(), True),
])

ITEMS_SCHEMA = StructType([
    StructField("id", IntegerType(), False),
    StructField("name", StringType(), False),
    StructField("api_name", StringType(), True),
    StructField("description", StringType(), True),
    StructField("is_battle", BooleanType(), True),
    StructField("mapped_type", StringType(), True),
])


# ============================================================
# ETL 函数
# ============================================================

def load_json(filepath: str) -> list[dict]:
    """加载 JSON 文件中的数组数据。"""
    with open(filepath, 'r', encoding='utf-8') as f:
        data = json.load(f)
    # 自动检测包装键: {"species": [...], "moves": [...], etc.}
    if isinstance(data, dict):
        # 找第一个 list 类型的值
        for key, val in data.items():
            if isinstance(val, list) and len(val) > 0:
                print(f"  检测到包装键: '{key}' ({len(val)} 条记录)")
                return val
    if isinstance(data, list):
        return data
    raise ValueError(f"无法解析 JSON 格式: {filepath}")


def species_row(item: dict) -> dict:
    """将 species JSON 记录转换为 Parquet row。"""
    stats = item.get("baseStats", [0]*6)
    return {
        "id": item.get("id", 0),
        "name": item.get("name", ""),
        "type1": item.get("type1"),
        "type2": item.get("type2"),
        "base_hp": stats[0] if len(stats) > 0 else 0,
        "base_atk": stats[1] if len(stats) > 1 else 0,
        "base_def": stats[2] if len(stats) > 2 else 0,
        "base_spa": stats[3] if len(stats) > 3 else 0,
        "base_spd": stats[4] if len(stats) > 4 else 0,
        "base_spe": stats[5] if len(stats) > 5 else 0,
        "height": item.get("height"),
        "weight": item.get("weight"),
        "abilities": item.get("abilities", []),
        "hidden_ability_id": item.get("hiddenAbilityID"),
        "learnable_moves": item.get("learnableMoves", []),
        "egg_group1": item.get("eggGroups", [None])[0] if item.get("eggGroups") else None,
        "egg_group2": item.get("eggGroups", [None, None])[1] if len(item.get("eggGroups", [])) > 1 else None,
        "evolution_level": item.get("evolutionLevel"),
        "next_evolution_id": item.get("nextEvolutionID"),
        "male_ratio": item.get("maleRatio"),
    }


def moves_row(item: dict) -> dict:
    return {
        "id": item.get("id", 0),
        "name": item.get("name", ""),
        "type": item.get("type"),
        "category": item.get("category"),
        "power": item.get("power"),
        "accuracy": item.get("accuracy"),
        "pp": item.get("pp"),
        "priority": item.get("priority", 0),
        "target": item.get("target"),
        "effect": str(item.get("effect", ""))[:256],
        "effect_chance": item.get("effectChance"),
        "description": item.get("description"),
    }


def abilities_row(item: dict) -> dict:
    return {
        "id": item.get("id", 0),
        "name": item.get("name", ""),
        "api_name": item.get("apiName"),
        "description": item.get("description"),
        "mapped_type": item.get("mappedType"),
    }


def items_row(item: dict) -> dict:
    return {
        "id": item.get("id", 0),
        "name": item.get("name", ""),
        "api_name": item.get("apiName"),
        "description": item.get("description"),
        "is_battle": item.get("isBattle", False),
        "mapped_type": item.get("mappedType"),
    }


# ============================================================
# 主流程
# ============================================================

# 映射: JSON 文件名 → (schema, row_converter, parquet_name)
DATA_FILES = {
    "species.json":   (SPECIES_SCHEMA,   species_row,   "species"),
    "moves.json":     (MOVES_SCHEMA,     moves_row,     "moves"),
    "abilities.json": (ABILITIES_SCHEMA, abilities_row, "abilities"),
    "items.json":     (ITEMS_SCHEMA,     items_row,     "items"),
}


def main():
    parser = argparse.ArgumentParser(description="JSON → Parquet ETL")
    parser.add_argument("--data-dir", default="data",
                        help="JSON 文件目录 (默认: data/)")
    parser.add_argument("--output", default=None,
                        help="输出路径: 本地目录 或 hdfs://...")
    parser.add_argument("--hdfs-namenode", default="hdfs://100.107.105.99:8020",
                        help="HDFS NameNode 地址")
    parser.add_argument("--local", action="store_true",
                        help="输出到本地文件系统")
    args = parser.parse_args()

    # 确定输出路径
    hdfs_base = f"{args.hdfs_namenode}/user/pokemon/static"
    local_base = args.output or "output"

    # 创建 SparkSession
    spark = SparkSession.builder \
        .appName("PokemonSimulator-ETL") \
        .config("spark.sql.adaptive.enabled", "true") \
        .getOrCreate()

    print(f"=== PokemonSimulator JSON → Parquet ETL ===")
    print(f"时间: {datetime.now().isoformat()}")
    print(f"输出: {'本地' if args.local else hdfs_base}")
    print()

    processed = 0
    for filename, (schema, row_func, parquet_name) in DATA_FILES.items():
        filepath = os.path.join(args.data_dir, filename)

        if not os.path.exists(filepath):
            print(f"  ⚠ 跳过 (文件不存在): {filepath}")
            continue

        print(f"处理: {filename} → {parquet_name}.parquet")

        # 加载 JSON
        raw_data = load_json(filepath)
        print(f"  加载 {len(raw_data)} 条记录")

        # 转换
        rows = [row_func(item) for item in raw_data]
        df = spark.createDataFrame(rows, schema=schema)

        # 写入
        if args.local:
            output_path = os.path.join(local_base, f"{parquet_name}.parquet")
            df.write.mode("overwrite").parquet(output_path)
            print(f"  ✓ 写入本地: {output_path}")
        else:
            output_path = f"{hdfs_base}/{parquet_name}.parquet"
            df.repartition(1).write.mode("overwrite").parquet(output_path)
            print(f"  ✓ 写入 HDFS: {output_path}")

        processed += 1

    spark.stop()

    print()
    print(f"=== ETL 完成 ({processed}/{len(DATA_FILES)} 文件处理) ===")


if __name__ == "__main__":
    main()
