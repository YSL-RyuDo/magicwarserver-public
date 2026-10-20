-- Synthetic regression fixture only; not a production migration.
CREATE DATABASE arcana_test CHARACTER SET utf8mb4;
USE arcana_test;
CREATE TABLE user_account (
 id VARCHAR(32) PRIMARY KEY, hash VARCHAR(128) NOT NULL, salt VARCHAR(64) NOT NULL,
 nickname VARCHAR(32) NOT NULL UNIQUE
);
CREATE TABLE user_info (
 user_id VARCHAR(32) PRIMARY KEY, level INT NOT NULL DEFAULT 1,
 exp INT NOT NULL DEFAULT 0, win INT NOT NULL DEFAULT 0, lose INT NOT NULL DEFAULT 0
);
CREATE TABLE user_rank (
 user_id VARCHAR(32) PRIMARY KEY, mmr INT NOT NULL DEFAULT 1000,
 `rank` INT NOT NULL DEFAULT 0, rank_score INT NOT NULL DEFAULT 0
);
CREATE TABLE user_magic (
 user_id VARCHAR(32) PRIMARY KEY, magic0 INT, magic1 INT, magic2 INT, magic3 INT
);
CREATE TABLE rank_table (
 `rank` INT PRIMARY KEY, name VARCHAR(15), min_score INT, max_score INT
);
INSERT INTO rank_table VALUES (0,'TestRank',0,999);
CREATE TABLE card_table (
 id INT PRIMARY KEY, name VARCHAR(32), attack INT, cooldown INT, cost INT,
 effect_type VARCHAR(16), speed FLOAT, radius FLOAT, duration FLOAT, amount INT
);
INSERT INTO card_table VALUES
 (0,'Fixture0',1,1,1,'Test',1,1,1,1),
 (1,'Fixture1',1,1,1,'Test',1,1,1,1),
 (2,'Fixture2',1,1,1,'Test',1,1,1,1),
 (3,'Fixture3',1,1,1,'Test',1,1,1,1),
 (4,'Fixture4',1,1,1,'Test',1,1,1,1);
