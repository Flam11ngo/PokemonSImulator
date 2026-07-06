#!/usr/bin/env python3
"""Migrate Smogon stats from SQLite → MySQL.
Usage: python scripts/py/migrate_smogon_to_mysql.py [--host HOST] [--batch N]

Before running:
  mysql -h HOST -u root -p < scripts/sql/smogon_schema.sql
"""
import sqlite3, sys, os, time, argparse
from pathlib import Path

# Try MySQL connector
try:
    import mysql.connector
    from mysql.connector import Error
    HAVE_MYSQL = True
except ImportError:
    print("[!] mysql-connector-python not installed. Run: pip install mysql-connector-python")
    HAVE_MYSQL = False

SRC = Path(__file__).resolve().parent.parent.parent / "smogon_stats" / "gen91v1_stats.sqlite"

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default=os.getenv("MYSQL_HOST", "192.168.88.129"))
    ap.add_argument("--port", type=int, default=int(os.getenv("MYSQL_PORT", "3306")))
    ap.add_argument("--user", default=os.getenv("MYSQL_USER", "root"))
    ap.add_argument("--password", default=os.getenv("MYSQL_PASSWORD", ""))
    ap.add_argument("--database", default="pokemon_stats")
    ap.add_argument("--batch", type=int, default=5000, help="Rows per INSERT batch")
    ap.add_argument("--tables", default="all", help="Comma-separated table names or 'all'")
    args = ap.parse_args()

    if not HAVE_MYSQL:
        sys.exit(1)

    print(f"Source: {SRC} ({SRC.stat().st_size / 1024 / 1024:.0f}MB)")
    print(f"Target: mysql://{args.user}@{args.host}:{args.port}/{args.database}")

    src_db = sqlite3.connect(str(SRC))
    src_db.row_factory = sqlite3.Row

    conn = mysql.connector.connect(
        host=args.host, port=args.port,
        user=args.user, password=args.password,
        database=args.database, charset='utf8mb4'
    )
    cur = conn.cursor()

    # ── Mapping: SQLite table → MySQL table, column mapping ──
    migrations = {
        "name_mapping": {
            "table": "name_mapping",
            "cols": ["english", "chinese"],
            "from": lambda r: [r["english"], r["chinese"]],
            "dedup": True  # INSERT IGNORE for duplicates
        },
        "mon": {
            "table": "mon_usage",
            "cols": ["mon", "source", "time_bucket", "rating", "usage_pct", "viability_ceiling"],
            "from": lambda r: [r["name"], r["source"] or "gen91v1", r["time_bucket"] or "0",
                             r["rating"] or 0, r["usage"] or 0, r["viability_ceiling"]]
        },
        "ability": {
            "table": "ability_usage",
            "cols": ["mon", "ability_name", "source", "time_bucket", "rating", "usage_pct"],
            "from": lambda r: [r["mon"], r["name"], r["source"] or "gen91v1",
                             r["time_bucket"] or "0", r["rating"] or 0, r["usage"] or 0]
        },
        "move": {
            "table": "move_usage",
            "cols": ["mon", "move_name", "source", "time_bucket", "rating", "usage_pct"],
            "from": lambda r: [r["mon"], r["name"], r["source"] or "gen91v1",
                             r["time_bucket"] or "0", r["rating"] or 0, r["usage"] or 0]
        },
        "item": {
            "table": "item_usage",
            "cols": ["mon", "item_name", "source", "time_bucket", "rating", "usage_pct"],
            "from": lambda r: [r["mon"], r["name"], r["source"] or "gen91v1",
                             r["time_bucket"] or "0", r["rating"] or 0, r["usage"] or 0]
        },
        "tera": {
            "table": "tera_usage",
            "cols": ["mon", "tera_type", "source", "time_bucket", "rating", "usage_pct"],
            "from": lambda r: [r["mon"], r["type"], r["source"] or "gen91v1",
                             r["time_bucket"] or "0", r["rating"] or 0, r["usage"] or 0]
        },
        "team": {
            "table": "teammate_usage",
            "cols": ["mon", "mate", "source", "time_bucket", "rating", "usage_pct"],
            "from": lambda r: [r["mon"], r["mate"], r["source"] or "gen91v1",
                             r["time_bucket"] or "0", r["rating"] or 0, r["usage"] or 0]
        },
        "cc": {
            "table": "matchup_stats",
            "cols": ["mon", "opp", "source", "time_bucket", "rating", "win_pct", "stddev"],
            "from": lambda r: [r["mon"], r["opp"], r["source"] or "gen91v1",
                             r["time_bucket"] or "0", r["rating"] or 0,
                             r["percentage"] or 0, r["stddev"] or 0]
        },
        "spread": {
            "table": "spread_usage",
            "cols": ["mon", "nature", "evs", "source", "time_bucket", "rating", "usage_pct"],
            "from": lambda r: [r["mon"], r["nature"], r["evs"],
                             r["source"] or "gen91v1", r["time_bucket"] or "0",
                             r["rating"] or 0, r["usage"] or 0]
        },
    }

    tbl_filter = args.tables.split(",") if args.tables != "all" else list(migrations.keys())

    for src_table in tbl_filter:
        if src_table not in migrations:
            print(f"  Skip unknown: {src_table}")
            continue

        meta = migrations[src_table]
        total = src_db.execute(f"SELECT COUNT(*) FROM {src_table}").fetchone()[0]
        print(f"\n--- {src_table} → {meta['table']} ({total:,} rows) ---")

        # Clear target table (skip for dedup tables)
        if not meta.get("dedup"):
            cur.execute(f"DELETE FROM {meta['table']}")
            conn.commit()

        placeholders = ",".join(["%s"] * len(meta["cols"]))
        cols_str = ",".join(meta["cols"])
        insert_kw = "INSERT IGNORE INTO" if meta.get("dedup") else "INSERT INTO"
        sql = f"{insert_kw} {meta['table']} ({cols_str}) VALUES ({placeholders})"

        rows = src_db.execute(f"SELECT * FROM {src_table}")
        batch, cum = [], 0
        t0 = time.time()

        for row in rows:
            batch.append(tuple(meta["from"](row)))
            if len(batch) >= args.batch:
                cur.executemany(sql, batch)
                conn.commit()
                cum += len(batch)
                elapsed = time.time() - t0
                rate = cum / elapsed if elapsed > 0 else 0
                pct = cum / total * 100
                print(f"\r  {cum:>10,} / {total:,} ({pct:.0f}%) {rate:,.0f} rows/s", end="", flush=True)
                batch = []

        if batch:
            cur.executemany(sql, batch)
            conn.commit()
            cum += len(batch)

        print(f"\r  {cum:>10,} / {total:,} (100%) done")

    cur.close(); conn.close(); src_db.close()
    print("\n✓ Migration complete")

if __name__ == "__main__":
    main()
