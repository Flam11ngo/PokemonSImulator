-- Smogon Stats → MySQL schema for distributed deployment
-- Source: gen91v1_stats.sqlite (Gen9 1v1 format)

CREATE DATABASE IF NOT EXISTS pokemon_stats CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE pokemon_stats;

-- ═══════════ Static / Reference ═══════════

-- Species name mapping (English → Chinese)
CREATE TABLE name_mapping (
    english VARCHAR(64) PRIMARY KEY,
    chinese VARCHAR(64) NOT NULL
) ENGINE=InnoDB;

-- ═══════════ Usage Stats ═══════════

-- Pokemon usage (weighted by rating bracket)
CREATE TABLE mon_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL,
    source VARCHAR(32) NOT NULL DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL COMMENT 'rating bracket or date range',
    rating INT NOT NULL DEFAULT 0 COMMENT 'Elo/GXE rating',
    usage_pct REAL NOT NULL DEFAULT 0 COMMENT 'usage percentage',
    viability_ceiling REAL COMMENT 'upper bound of viability range',
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_bucket (time_bucket),
    INDEX idx_rating (rating),
    INDEX idx_mon_bucket (mon, time_bucket)
) ENGINE=InnoDB;

-- Ability usage per Pokemon
CREATE TABLE ability_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL,
    ability_name VARCHAR(64) NOT NULL,
    source VARCHAR(32) DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL,
    rating INT DEFAULT 0,
    usage_pct REAL DEFAULT 0,
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_mon_bucket (mon, time_bucket)
) ENGINE=InnoDB;

-- Move usage per Pokemon
CREATE TABLE move_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL,
    move_name VARCHAR(64) NOT NULL,
    source VARCHAR(32) DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL,
    rating INT DEFAULT 0,
    usage_pct REAL DEFAULT 0,
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_mon_bucket (mon, time_bucket),
    INDEX idx_move (move_name)
) ENGINE=InnoDB;

-- Item usage per Pokemon
CREATE TABLE item_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL,
    item_name VARCHAR(64) NOT NULL,
    source VARCHAR(32) DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL,
    rating INT DEFAULT 0,
    usage_pct REAL DEFAULT 0,
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_mon_bucket (mon, time_bucket)
) ENGINE=InnoDB;

-- Tera type usage per Pokemon
CREATE TABLE tera_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL,
    tera_type VARCHAR(16) NOT NULL,
    source VARCHAR(32) DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL,
    rating INT DEFAULT 0,
    usage_pct REAL DEFAULT 0,
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_mon_bucket (mon, time_bucket)
) ENGINE=InnoDB;

-- ═══════════ Team / Spread / Matchup ═══════════

-- Teammate co-occurrence ("mon is used with mate")
CREATE TABLE teammate_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL,
    mate VARCHAR(64) NOT NULL,
    source VARCHAR(32) DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL,
    rating INT DEFAULT 0,
    usage_pct REAL DEFAULT 0,
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_mate (mate),
    INDEX idx_pair (mon, mate)
) ENGINE=InnoDB;

-- EV/Nature spread usage
CREATE TABLE spread_usage (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL,
    nature VARCHAR(16) NOT NULL,
    evs TEXT NOT NULL COMMENT 'EV distribution string',
    source VARCHAR(32) DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL,
    rating INT DEFAULT 0,
    usage_pct REAL DEFAULT 0,
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_mon_bucket (mon, time_bucket),
    INDEX idx_mon_nature (mon, nature)
) ENGINE=InnoDB;

-- Check/Counter matchups (mon vs opp win/loss percentages)
CREATE TABLE matchup_stats (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    mon VARCHAR(64) NOT NULL COMMENT 'the Pokemon being checked',
    opp VARCHAR(64) NOT NULL COMMENT 'the opposing Pokemon',
    source VARCHAR(32) DEFAULT 'gen91v1',
    time_bucket VARCHAR(16) NOT NULL,
    rating INT DEFAULT 0,
    win_pct REAL DEFAULT 0 COMMENT 'mon wins this matchup X% of the time',
    stddev REAL COMMENT 'standard deviation',
    imported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_mon (mon),
    INDEX idx_opp (opp),
    INDEX idx_pair (mon, opp),
    INDEX idx_mon_winrate (mon, win_pct)
) ENGINE=InnoDB;

-- ═══════════ Our Battle Stats (dynamic, populated by stream_consumer) ═══════════

CREATE TABLE battle_stats (
    stat_type VARCHAR(32) NOT NULL COMMENT 'species_usage, species_winrate, move_usage, etc.',
    entity_id INT NOT NULL COMMENT 'species_id or move_id or item_id',
    value REAL NOT NULL DEFAULT 0,
    count INT NOT NULL DEFAULT 0,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (stat_type, entity_id)
) ENGINE=InnoDB;

-- ═══════════ Materialized Views (for fast TeamBuilder queries) ═══════════

-- Top moves per Pokemon (pre-computed, refreshed periodically)
CREATE TABLE top_moves (
    mon VARCHAR(64) NOT NULL,
    move_name VARCHAR(64) NOT NULL,
    usage_pct REAL NOT NULL,
    rank_n INT NOT NULL DEFAULT 1,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (mon, move_name)
) ENGINE=InnoDB;

-- Top teammates per Pokemon
CREATE TABLE top_teammates (
    mon VARCHAR(64) NOT NULL,
    mate VARCHAR(64) NOT NULL,
    usage_pct REAL NOT NULL,
    rank_n INT NOT NULL DEFAULT 1,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (mon, mate)
) ENGINE=InnoDB;

-- Best counters per Pokemon (what beats this mon)
CREATE TABLE best_counters (
    target_mon VARCHAR(64) NOT NULL COMMENT 'Pokemon to counter',
    counter_mon VARCHAR(64) NOT NULL COMMENT 'Pokemon that wins vs target',
    win_pct REAL NOT NULL DEFAULT 0,
    rank_n INT NOT NULL DEFAULT 1,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (target_mon, counter_mon)
) ENGINE=InnoDB;
