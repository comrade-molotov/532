-- ============================================================
--  SA-MP Freeroam 0.3.7 - მონაცემთა ბაზის სქემა
--
--  გაუშვით ერთხელ phpMyAdmin-ში / MySQL კონსოლში / HeidiSQL-ში.
--  გამეორებაც უსაფრთხოა: ყველა ცხრილზე არის IF NOT EXISTS.
--
--  gamemode თვითონაც ქმნის ამ ცხრილებს გაშვებისას, ასე რომ
--  ეს ფაილი საჭიროა მხოლოდ ბაზის შესაქმნელად (ან რომ ხელით
--  ნახოთ/შეცვალოთ სქემა).
-- ============================================================

CREATE DATABASE IF NOT EXISTS `freeroam`
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_general_ci;

USE `freeroam`;

-- ------------------------------------------------------------
--  ანგარიშები (რეგისტრაცია / ავტორიზაცია)
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `accounts` (
    `id`          INT UNSIGNED     NOT NULL AUTO_INCREMENT,
    `username`    VARCHAR(24)      NOT NULL,
    `password`    CHAR(64)         NOT NULL,           -- SHA-256 ჰეშის hex, არასდროს plain text
    `admin_level` TINYINT UNSIGNED NOT NULL DEFAULT 0,  -- 0 = მოთამაშე, 1-3 = ადმინი
    `money`       INT              NOT NULL DEFAULT 0,
    `score`       INT              NOT NULL DEFAULT 0,
    `created_at`  TIMESTAMP        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    UNIQUE KEY `uq_username` (`username`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ------------------------------------------------------------
--  კაზინოს ტრანზაქციების ლოგი (blackjack / roulette / slots / poker)
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `casino_log` (
    `id`         INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `account_id` INT UNSIGNED NOT NULL,
    `game`       VARCHAR(16)  NOT NULL,               -- 'blackjack', 'roulette', 'slots', 'poker'
    `amount`     INT          NOT NULL,               -- დადებული თანხა
    `balance`    INT          NOT NULL,               -- ბალანსი ხელის დასრულების შემდეგ
    `created_at` TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_account` (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- ------------------------------------------------------------
--  ბანები (ადმინის /ban)
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `bans` (
    `id`         INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `username`   VARCHAR(24)  NOT NULL,
    `ip`         VARCHAR(16)  NOT NULL,
    `reason`     VARCHAR(128) NOT NULL,
    `admin`      VARCHAR(24)  NOT NULL,
    `created_at` TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_username` (`username`),
    KEY `idx_ip` (`ip`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
