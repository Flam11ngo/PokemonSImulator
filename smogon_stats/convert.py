import os
import re
import json
import sqlite3
from pathlib import Path

# Paths
WORKSPACE_DIR = Path(r"D:\temp\shixun\python\smogon_stats")
DB_PATH = WORKSPACE_DIR / "gen91v1_stats.sqlite"

# Regex to extract period and rating from filename, e.g., "2025-07_gen91v1-1500.json"
FILE_PATTERN = re.compile(r"^(\d{4}-\d{2})_gen91v1-(\d+)\.json$")

def create_tables(conn: sqlite3.Connection):
    cursor = conn.cursor()
    
    # 🏆 核心升级 1：引入 source 区分 'smogon'(大网离线) 与 'simulator'(本地实时)
    # 🏆 核心升级 2：将 period 泛化为 time_bucket，兼容 '2026-06' 和 '2026-06-30 09:00'
    # 🏆 核心升级 3：死锁复合主键，彻底堵死重复数据
    
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS mon (
        name TEXT NOT NULL,
        source TEXT NOT NULL,       
        time_bucket TEXT NOT NULL,  
        rating INTEGER NOT NULL,
        usage REAL,
        viability_ceiling REAL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (name, source, time_bucket, rating)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS ability (
        mon TEXT NOT NULL,
        name TEXT NOT NULL,
        source TEXT NOT NULL,
        time_bucket TEXT NOT NULL,
        rating INTEGER NOT NULL,
        usage REAL NOT NULL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (mon, name, source, time_bucket, rating)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS move (
        mon TEXT NOT NULL,
        name TEXT NOT NULL,
        source TEXT NOT NULL,
        time_bucket TEXT NOT NULL,
        rating INTEGER NOT NULL,
        usage REAL NOT NULL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (mon, name, source, time_bucket, rating)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS item (
        mon TEXT NOT NULL,
        name TEXT NOT NULL,
        source TEXT NOT NULL,
        time_bucket TEXT NOT NULL,
        rating INTEGER NOT NULL,
        usage REAL NOT NULL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (mon, name, source, time_bucket, rating)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS tera (
        mon TEXT NOT NULL,
        type TEXT NOT NULL,
        source TEXT NOT NULL,
        time_bucket TEXT NOT NULL,
        rating INTEGER NOT NULL,
        usage REAL NOT NULL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (mon, type, source, time_bucket, rating)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS team (
        mon TEXT NOT NULL,
        mate TEXT NOT NULL,
        source TEXT NOT NULL,
        time_bucket TEXT NOT NULL,
        rating INTEGER NOT NULL,
        usage REAL NOT NULL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (mon, mate, source, time_bucket, rating)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS cc (
        mon TEXT NOT NULL,
        opp TEXT NOT NULL,
        source TEXT NOT NULL,
        time_bucket TEXT NOT NULL,
        rating INTEGER NOT NULL,
        percentage REAL NOT NULL,
        stddev REAL NOT NULL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (mon, opp, source, time_bucket, rating)
    );
    """)

    cursor.execute("""
    CREATE TABLE IF NOT EXISTS spread (
        mon TEXT NOT NULL,
        nature TEXT NOT NULL,
        evs TEXT NOT NULL,
        source TEXT NOT NULL,
        time_bucket TEXT NOT NULL,
        rating INTEGER NOT NULL,
        usage REAL NOT NULL,
        imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        PRIMARY KEY (mon, nature, evs, source, time_bucket, rating)
    );
    """)
    
    # 🏆 升级索引：将 source 纳入聚集索引，看板在执行 WHERE source='simulator' 时速度会提升百倍
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_mon_lookup ON mon(source, time_bucket, rating);")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_ability_lookup ON ability(source, time_bucket, rating);")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_move_lookup ON move(source, time_bucket, rating);")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_item_lookup ON item(source, time_bucket, rating);")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_tera_lookup ON tera(source, time_bucket, rating);")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_team_lookup ON team(source, time_bucket, rating);")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_cc_lookup ON cc(source, time_bucket, rating);")
    cursor.execute("CREATE INDEX IF NOT EXISTS idx_spread_lookup ON spread(source, time_bucket, rating);")


def process_file(conn, file_path, period, rating):
    print(f"Processing {file_path.name} ({period} @ {rating})...")
    with open(file_path, "r", encoding="utf-8") as f:
        stats = json.load(f)
        
    data = stats.get("data", {})
    
    mon_rows = []
    ability_rows = []
    move_rows = []
    item_rows = []
    tera_rows = []
    team_rows = []
    cc_rows = []
    spread_rows = []
    
    source = "smogon"
    
    for mon, details in data.items():
        abilities = details.get("Abilities", {})
        # HACK: Sum abilities usage to proxy total mon appearances (mon_count)
        mon_count = float(sum(abilities.values()))
        if mon_count <= 0.0:
            mon_count = 1.0
            
        usage = details.get("usage")
        vc_list = details.get("Viability Ceiling")
        viability_ceiling = float(vc_list[1]) if vc_list and len(vc_list) > 1 else None
        
        # Insert main mon record
        mon_rows.append((mon, source, period, rating, usage, viability_ceiling))
        
        # Insert abilities
        for ability, val in abilities.items():
            ability_rows.append((mon, ability, source, period, rating, val / mon_count))
            
        # Insert moves
        moves = details.get("Moves", {})
        for move, val in moves.items():
            move_rows.append((mon, move, source, period, rating, val / mon_count))
            
        # Insert items
        items = details.get("Items", {})
        for item, val in items.items():
            item_rows.append((mon, item, source, period, rating, val / mon_count))
            
        # Insert tera types
        tera_types = details.get("Tera Types", {})
        for tera, val in tera_types.items():
            tera_rows.append((mon, tera.lower(), source, period, rating, val / mon_count))
            
        # Insert teammates
        teammates = details.get("Teammates", {})
        for mate, val in teammates.items():
            team_rows.append((mon, mate, source, period, rating, val / mon_count))
            
        # Insert checks and counters
        ccs = details.get("Checks and Counters", {})
        for opp, val in ccs.items():
            if isinstance(val, list) and len(val) >= 3:
                percentage = val[1]
                stddev = val[2]
            elif isinstance(val, dict):
                percentage = val.get("p")
                stddev = val.get("d")
            else:
                continue
                
            if percentage is not None and stddev is not None:
                cc_rows.append((mon, opp, source, period, rating, percentage, stddev))
                
        # Insert spreads
        spreads = details.get("Spreads", {})
        for spread, val in spreads.items():
            if ":" in spread:
                nature, evs = spread.split(":", 1)
            else:
                nature, evs = spread, ""
            spread_rows.append((mon, nature, evs, source, period, rating, val / mon_count))
            
    # Executing batch inserts inside transaction context manager
    with conn:
        cursor = conn.cursor()
        if mon_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO mon (name, source, time_bucket, rating, usage, viability_ceiling) VALUES (?, ?, ?, ?, ?, ?)",
                mon_rows
            )
        if ability_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO ability (mon, name, source, time_bucket, rating, usage) VALUES (?, ?, ?, ?, ?, ?)",
                ability_rows
            )
        if move_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO move (mon, name, source, time_bucket, rating, usage) VALUES (?, ?, ?, ?, ?, ?)",
                move_rows
            )
        if item_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO item (mon, name, source, time_bucket, rating, usage) VALUES (?, ?, ?, ?, ?, ?)",
                item_rows
            )
        if tera_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO tera (mon, type, source, time_bucket, rating, usage) VALUES (?, ?, ?, ?, ?, ?)",
                tera_rows
            )
        if team_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO team (mon, mate, source, time_bucket, rating, usage) VALUES (?, ?, ?, ?, ?, ?)",
                team_rows
            )
        if cc_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO cc (mon, opp, source, time_bucket, rating, percentage, stddev) VALUES (?, ?, ?, ?, ?, ?, ?)",
                cc_rows
            )
        if spread_rows:
            cursor.executemany(
                "INSERT OR REPLACE INTO spread (mon, nature, evs, source, time_bucket, rating, usage) VALUES (?, ?, ?, ?, ?, ?, ?)",
                spread_rows
            )


def main():
    if DB_PATH.exists():
        print(f"Removing existing database: {DB_PATH}")
        os.remove(DB_PATH)
        
    conn = sqlite3.connect(DB_PATH)
    # Enable performance optimization PRAGMAs
    conn.execute("PRAGMA journal_mode=WAL;")
    conn.execute("PRAGMA synchronous=NORMAL;")
    conn.execute("PRAGMA temp_store=MEMORY;")
    conn.execute("PRAGMA cache_size=-64000;")  # 64MB Cache
    
    create_tables(conn)
    
    # Scan workspace directory for JSON files
    json_files = []
    for entry in WORKSPACE_DIR.iterdir():
        if entry.is_file():
            match = FILE_PATTERN.match(entry.name)
            if match:
                period, rating = match.group(1), int(match.group(2))
                json_files.append((entry, period, rating))
                
    # Sort files chronologically and by rating to insert in order
    json_files.sort(key=lambda x: (x[1], x[2]))
    
    print(f"Found {len(json_files)} matching Smogon stats files.")
    for file_path, period, rating in json_files:
        process_file(conn, file_path, period, rating)
        
    # Run VACUUM and ANALYZE to optimize database size and query plans
    print("Optimizing database...")
    conn.execute("VACUUM;")
    conn.execute("ANALYZE;")
    conn.close()
    print("Done! Database generated at:", DB_PATH)

if __name__ == "__main__":
    main()
