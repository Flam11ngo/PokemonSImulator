import os
import sqlite3
from typing import Optional
from fastapi import FastAPI, HTTPException, Query
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI(title="Smogon Stats Dashboard API")

# Enable CORS for development
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

DB_PATH = os.path.join(os.path.dirname(__file__), "gen91v1_stats.sqlite")

def get_db():
    if not os.path.exists(DB_PATH):
        raise HTTPException(status_code=500, detail="Database file not found. Please run convert.py first.")
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

# API to get filter options
@app.get("/api/filters")
def get_filters():
    try:
        conn = get_db()
        cursor = conn.cursor()
        
        # Get unique sources
        cursor.execute("SELECT DISTINCT source FROM mon ORDER BY source")
        sources = [row["source"] for row in cursor.fetchall()]
        
        # Get unique time buckets
        cursor.execute("SELECT DISTINCT time_bucket FROM mon ORDER BY time_bucket DESC")
        time_buckets = [row["time_bucket"] for row in cursor.fetchall()]
        
        # Get unique ratings
        cursor.execute("SELECT DISTINCT rating FROM mon ORDER BY rating")
        ratings = [row["rating"] for row in cursor.fetchall()]
        
        conn.close()
        return {
            "sources": sources,
            "time_buckets": time_buckets,
            "ratings": ratings
        }
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# API to get ranked pokemon list
@app.get("/api/pokemon")
def get_pokemon(
    source: str,
    time_bucket: str,
    rating: int,
    search: Optional[str] = None,
    limit: int = 50
):
    try:
        conn = get_db()
        cursor = conn.cursor()
        
        query = """
            SELECT m.name, m.usage, m.viability_ceiling, t.chinese AS chinese_name
            FROM mon m
            LEFT JOIN name_mapping t ON m.name = t.english
            WHERE m.source = ? AND m.time_bucket = ? AND m.rating = ?
        """
        params = [source, time_bucket, rating]
        
        if search:
            query += " AND (m.name LIKE ? OR t.chinese LIKE ?)"
            params.extend([f"%{search}%", f"%{search}%"])
            
        query += " ORDER BY m.usage DESC LIMIT ?"
        params.append(limit)
        
        cursor.execute(query, params)
        pokemon_list = [dict(row) for row in cursor.fetchall()]
        conn.close()
        return pokemon_list
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# API to get detail stats of a specific pokemon
@app.get("/api/pokemon/{name}")
def get_pokemon_detail(
    name: str,
    source: str,
    time_bucket: str,
    rating: int
):
    try:
        conn = get_db()
        cursor = conn.cursor()
        
        # 1. Main mon info
        cursor.execute(
            """
            SELECT m.name, m.usage, m.viability_ceiling, t.chinese AS chinese_name 
            FROM mon m 
            LEFT JOIN name_mapping t ON m.name = t.english
            WHERE m.name = ? AND m.source = ? AND m.time_bucket = ? AND m.rating = ?
            """,
            (name, source, time_bucket, rating)
        )
        mon_row = cursor.fetchone()
        if not mon_row:
            conn.close()
            raise HTTPException(status_code=404, detail=f"Pokémon '{name}' not found under current filters.")
            
        mon_info = dict(mon_row)
        
        # Helper to fetch related lists
        def fetch_related(table_name, select_cols="name, usage", order_by="usage DESC"):
            cursor.execute(
                f"SELECT {select_cols} FROM {table_name} WHERE mon = ? AND source = ? AND time_bucket = ? AND rating = ? ORDER BY {order_by}",
                (name, source, time_bucket, rating)
            )
            return [dict(row) for row in cursor.fetchall()]
            
        abilities = fetch_related("ability")
        moves = fetch_related("move")
        items = fetch_related("item")
        teras = fetch_related("tera", select_cols="type, usage")
        spreads = fetch_related("spread", select_cols="nature, evs, usage")
        
        # Fetch teammates with Chinese name mapping
        cursor.execute(
            """
            SELECT t.mate, t.usage, nm.chinese AS chinese_name
            FROM team t
            LEFT JOIN name_mapping nm ON t.mate = nm.english
            WHERE t.mon = ? AND t.source = ? AND t.time_bucket = ? AND t.rating = ?
            ORDER BY t.usage DESC
            """,
            (name, source, time_bucket, rating)
        )
        teammates = [dict(row) for row in cursor.fetchall()]
        
        # Checks and Counters with Chinese name mapping
        cursor.execute(
            """
            SELECT c.opp, c.percentage, c.stddev, nm.chinese AS chinese_name
            FROM cc c
            LEFT JOIN name_mapping nm ON c.opp = nm.english
            WHERE c.mon = ? AND c.source = ? AND c.time_bucket = ? AND c.rating = ?
            ORDER BY c.percentage DESC
            """,
            (name, source, time_bucket, rating)
        )
        ccs = [dict(row) for row in cursor.fetchall()]
        
        conn.close()
        
        return {
            "info": mon_info,
            "abilities": abilities,
            "moves": moves,
            "items": items,
            "teras": teras,
            "teammates": teammates,
            "ccs": ccs,
            "spreads": spreads
        }
    except HTTPException:
        raise
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

# Mount static folder and serve index.html
static_dir = os.path.join(os.path.dirname(__file__), "static")
if not os.path.exists(static_dir):
    os.makedirs(static_dir)

app.mount("/static", StaticFiles(directory=static_dir), name="static")

@app.get("/")
def read_index():
    index_path = os.path.join(static_dir, "index.html")
    if os.path.exists(index_path):
        return FileResponse(index_path)
    return {"message": "Server is running, but static/index.html is missing. Please build the frontend."}
