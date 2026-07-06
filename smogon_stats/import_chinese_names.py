import urllib.request
import json
import sqlite3
import os
import re

DB_PATH = os.path.join(os.path.dirname(__file__), "gen91v1_stats.sqlite")

SUFFIX_TRANSLATIONS = {
    "Unbound": "解放",
    "Confined": "惩戒",
    "Therian": "灵兽",
    "Incarnate": "化身",
    "Wash": "清洗",
    "Heat": "加热",
    "Mow": "割草",
    "Frost": "结冰",
    "Fan": "旋转",
    "Alola": "阿罗拉",
    "Galar": "伽勒尔",
    "Hisui": "洗翠",
    "Paldea": "帕底亚",
    "Mega": "超级",
    "Primal": "原始",
    "Origin": "起源",
    "Crowned": "王之御",
    "Black": "暗黑",
    "White": "焰白",
    "Ice": "白马",
    "Shadow": "黑马",
    "Rapid-Strike": "连击流",
    "Single-Strike": "一击流",
    "Sandy": "沙尘",
    "Trash": "垃圾",
    "Plant": "草木",
    "Sunny": "晴天",
    "Rainy": "雨天",
    "Snowy": "雪天",
    "Blade": "刀锋",
    "Shield": "盾牌",
    "Active": "活跃",
    "Dusk": "黄昏",
    "Midday": "白昼",
    "Midnight": "黑夜",
    "School": "鱼群",
    "Busted": "画皮",
    "Sensu": "轻盈",
    "Pom-Pom": "啪啪",
    "Pa'u": "呼啦",
    "Baile": "热辣",
    "Small": "小",
    "Super": "特大",
    "Large": "大",
    "Gorging": "一口吞",
    "Gulping": "吞食",
    "Hangry": "空腹",
    "Low-Key": "低调",
    "Amped": "高频",
    "Hero": "全能",
    "Three-Segment": "三节",
    "Two-Segment": "二节",
    "Roost": "停栖",
    "Female": "雌",
    "Male": "雄",
    "F": "雌",
    "M": "雄",
    "Bloodmoon": "赫月",
    "Aqua": "水澜",
    "Blaze": "火焰",
    "Combat": "斗战",
    "Wellspring": "水井",
    "Hearthflame": "火灶",
    "Cornerstone": "础石",
}

MANUAL_OVERRIDES = {
    "Zacian-Crowned": "苍响-剑之王",
    "Zamazenta-Crowned": "藏玛然特-盾之王",
    "Palafin-Hero": "海豚侠-全能形态",
    "Flabebe": "花蓓蓓",
    "Flabébé": "花蓓蓓",
}

def fetch_json(url):
    req = urllib.request.Request(
        url, 
        headers={'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64)'}
    )
    with urllib.request.urlopen(req) as response:
        return json.loads(response.read().decode('utf-8'))

def main():
    # Fetch English and Simplified Chinese names list from sindresorhus/pokemon
    en_url = "https://fastly.jsdelivr.net/gh/sindresorhus/pokemon@main/data/en.json"
    zh_url = "https://fastly.jsdelivr.net/gh/sindresorhus/pokemon@main/data/zh-hans.json"
    
    print("Fetching English name index...")
    en_names = fetch_json(en_url)
    print("Fetching Chinese name index...")
    zh_names = fetch_json(zh_url)
    
    if len(en_names) != len(zh_names):
        print(f"Warning: Array lengths do not match ({len(en_names)} vs {len(zh_names)})")
        
    # Map English to Chinese
    name_map = {}
    for eng, chi in zip(en_names, zh_names):
        name_map[eng.lower()] = chi
        name_map[eng] = chi

    # Open DB and find unique names
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    
    # Ensure mapping table exists
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS name_mapping (
        english TEXT PRIMARY KEY,
        chinese TEXT NOT NULL
    );
    """)
    
    # Get all unique names currently in the 'mon' table (these are what we actually need to translate!)
    cursor.execute("SELECT DISTINCT name FROM mon")
    db_names = [row[0] for row in cursor.fetchall()]
    print(f"Found {len(db_names)} unique Pokemon names in database.")
    
    translated_count = 0
    mapping_rows = []
    
    for name in db_names:
        # Check manual overrides first
        if name in MANUAL_OVERRIDES:
            mapping_rows.append((name, MANUAL_OVERRIDES[name]))
            translated_count += 1
            continue
            
        # Check exact case-insensitive match
        if name in name_map:
            mapping_rows.append((name, name_map[name]))
            translated_count += 1
            continue
        elif name.lower() in name_map:
            mapping_rows.append((name, name_map[name.lower()]))
            translated_count += 1
            continue
            
        # Handle hyphenated suffixes (e.g. Hoopa-Unbound, Rotom-Wash)
        if "-" in name:
            parts = name.split("-")
            base = parts[0]
            # Try to translate base name
            base_zh = name_map.get(base) or name_map.get(base.lower())
            if base_zh:
                # Translate suffixes
                zh_parts = [base_zh]
                valid_split = True
                for p in parts[1:]:
                    if p in SUFFIX_TRANSLATIONS:
                        zh_parts.append(SUFFIX_TRANSLATIONS[p])
                    else:
                        # If a suffix isn't translated, just keep the English suffix part
                        zh_parts.append(p)
                
                translated_name = "-".join(zh_parts)
                mapping_rows.append((name, translated_name))
                translated_count += 1
                continue
                
        # If still no match, keep it as is or log it
        print(f"No translation found for: {name} (using fallback)")
        mapping_rows.append((name, name))
        
    print(f"Translated {translated_count} / {len(db_names)} Pokemon names.")
    
    cursor.executemany(
        "INSERT OR REPLACE INTO name_mapping (english, chinese) VALUES (?, ?)",
        mapping_rows
    )
    
    # Also add standard checks & counters names if any
    cursor.execute("SELECT DISTINCT opp FROM cc")
    cc_names = [row[0] for row in cursor.fetchall()]
    cc_mapping_rows = []
    for name in cc_names:
        if name in name_map:
            cc_mapping_rows.append((name, name_map[name]))
        elif name.lower() in name_map:
            cc_mapping_rows.append((name, name_map[name.lower()]))
        elif "-" in name:
            parts = name.split("-")
            base = parts[0]
            base_zh = name_map.get(base) or name_map.get(base.lower())
            if base_zh:
                zh_parts = [base_zh]
                for p in parts[1:]:
                    zh_parts.append(SUFFIX_TRANSLATIONS.get(p, p))
                cc_mapping_rows.append((name, "-".join(zh_parts)))
            else:
                cc_mapping_rows.append((name, name))
        else:
            cc_mapping_rows.append((name, name))
            
    cursor.executemany(
        "INSERT OR REPLACE INTO name_mapping (english, chinese) VALUES (?, ?)",
        cc_mapping_rows
    )
    
    conn.commit()
    conn.close()
    print("Full name mapping database populated successfully!")

if __name__ == "__main__":
    main()
