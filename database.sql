-- ============================================================
--  SA-MP Freeroam - database schema
--  Stage 1: accounts only
--
--  Run this once in phpMyAdmin / MySQL console / HeidiSQL.
--  The gamemode also creates the accounts table on startup
--  (CREATE TABLE IF NOT EXISTS), so this file is here mainly
--  as documentation and for creating the database itself.
-- ============================================================

CREATE DATABASE IF NOT EXISTS `freeroam`
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_general_ci;

USE `freeroam`;

CREATE TABLE IF NOT EXISTS `accounts` (
    `id`          INT UNSIGNED    NOT NULL AUTO_INCREMENT,
    `username`    VARCHAR(24)     NOT NULL,
    `password`    CHAR(64)        NOT NULL,           -- SHA256 hex hash, never plain text
    `admin_level` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `money`       INT             NOT NULL DEFAULT 0,
    `score`       INT             NOT NULL DEFAULT 0,
    `created_at`  TIMESTAMP       NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Later stages (do NOT create these yet):
--   vehicles    - player owned vehicles     (stage 3)
--   casino_log  - casino transactions      (stage 4)
--   bans        - ban system               (admin stage)
