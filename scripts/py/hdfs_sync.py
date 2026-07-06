#!/usr/bin/env python3
"""Sync battle data from local logs to HDFS. Run periodically or after each battle."""
import os, subprocess, time, glob
from pathlib import Path

HDFS_HOST = os.getenv("HDFS_HOST", "192.168.88.129")
HDFS_USER = os.getenv("HDFS_USER", "hadoop")
HDFS_BASE = os.getenv("HDFS_BASE", "/battle_data")
LOCAL_BASE = Path(__file__).resolve().parent.parent.parent / "data" / "logs"

SSH_CMD = ["ssh", f"{HDFS_USER}@{HDFS_HOST}"]
SCP_CMD = ["scp", "-r"]

def ssh_run(cmd: str):
    """Run command on HDFS host."""
    full = SSH_CMD + [cmd]
    r = subprocess.run(full, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"[hdfs] SSH err: {r.stderr.strip()}")
    return r.stdout.strip()

def ensure_dir(hdfs_path: str):
    ssh_run(f"hdfs dfs -mkdir -p {hdfs_path}")

def upload_file(local_path: str, hdfs_path: str):
    """Upload a single file to HDFS."""
    target = f"{HDFS_USER}@{HDFS_HOST}:{hdfs_path}"
    r = subprocess.run(SCP_CMD + [local_path, target], capture_output=True, text=True)
    if r.returncode != 0:
        print(f"[hdfs] SCP err: {r.stderr.strip()}")
        return False
    return True

def sync_battle(battle_dir: Path):
    """Upload one battle directory to HDFS."""
    bid = battle_dir.name  # e.g., "2026-07-02/04-30-15_abc123"
    hdfs_dir = f"{HDFS_BASE}/{bid}"
    ensure_dir(hdfs_dir)

    for sub in ["input", "output"]:
        local_sub = battle_dir / sub
        if local_sub.exists():
            hdfs_sub = f"{hdfs_dir}/{sub}"
            ensure_dir(hdfs_sub)
            for f in local_sub.iterdir():
                if f.is_file():
                    upload_file(str(f), f"{hdfs_sub}/{f.name}")

    # Also upload battle_summary.json
    summary = battle_dir / "battle_summary.json"
    if summary.exists():
        upload_file(str(summary), f"{hdfs_dir}/battle_summary.json")

    print(f"[hdfs] Synced {bid}")

def sync_all():
    """Sync all un-synced battle directories."""
    if not LOCAL_BASE.exists():
        print("[hdfs] No local logs yet")
        return

    # Track synced dirs
    synced_file = LOCAL_BASE / ".hdfs_synced"
    synced = set()
    if synced_file.exists():
        synced = set(synced_file.read_text().splitlines())

    for day_dir in sorted(LOCAL_BASE.iterdir()):
        if not day_dir.is_dir():
            continue
        for battle_dir in sorted(day_dir.iterdir()):
            if not battle_dir.is_dir():
                continue
            bid = f"{day_dir.name}/{battle_dir.name}"
            if bid in synced:
                continue
            try:
                sync_battle(battle_dir)
                synced.add(bid)
                synced_file.write_text("\n".join(sorted(synced)))
            except Exception as e:
                print(f"[hdfs] Failed {bid}: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        # Sync specific directory
        sync_battle(Path(sys.argv[1]))
    else:
        sync_all()
        print("[hdfs] Sync complete")
