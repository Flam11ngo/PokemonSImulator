#!/bin/bash
# Sync VM's gen91v1_stats.sqlite into local smogon_stats DB.
# Run from repo root. Requires: ssh hadoop@myz, sqlite3.
set -e

LOCAL="smogon_stats/gen91v1_stats.sqlite"
LOCAL_BAK="smogon_stats/gen91v1_stats.sqlite.bak.$(date +%Y%m%d_%H%M%S)"

echo "=== Smogon Stats Sync ==="
echo "VM: hadoop@myz:~/temp/gen91v1_stats.sqlite"
echo "Local: $LOCAL"

# 1. Pull VM DB to temp
echo ""
echo "[1/4] Pulling VM DB..."
TMP_VM="$(mktemp /tmp/vm_smogon_XXXXXX.sqlite)"
scp hadoop@myz:~/temp/gen91v1_stats.sqlite "$TMP_VM"
echo "  → $TMP_VM ($(stat -c%s "$TMP_VM" 2>/dev/null || ls -lh "$TMP_VM" | awk '{print $5}')B)"

# 2. Backup local
echo ""
echo "[2/4] Backing up local..."
cp "$LOCAL" "$LOCAL_BAK"
echo "  → $LOCAL_BAK"

# 3. Merge: INSERT OR IGNORE rows from VM into local
echo ""
echo "[3/4] Merging..."
TABLES="mon ability move item tera team cc spread"

for t in $TABLES; do
  before=$(sqlite3 "$LOCAL" "SELECT COUNT(*) FROM $t")

  # Get the column list (all columns)
  cols=$(sqlite3 "$LOCAL" "SELECT group_concat(name) FROM pragma_table_info('$t')")

  sqlite3 "$LOCAL" <<SQL
ATTACH DATABASE '$TMP_VM' AS vm;
INSERT OR IGNORE INTO main.$t ($cols) SELECT $cols FROM vm.$t;
DETACH DATABASE vm;
SQL

  after=$(sqlite3 "$LOCAL" "SELECT COUNT(*) FROM $t")
  added=$((after - before))
  printf "  %-12s %6d → %-6d (+%d)\n" "$t" "$before" "$after" "$added"
done

# 4. Cleanup
echo ""
echo "[4/4] Cleanup..."
rm -f "$TMP_VM"
echo "  Temp file removed."

echo ""
echo "=== Sync done! ==="
echo "Backup: $LOCAL_BAK"
