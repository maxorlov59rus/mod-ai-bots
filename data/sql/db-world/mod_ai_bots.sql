-- mod-ai-bots — модуль AI-ботов для AzerothCore
-- База: world
--
-- Таблица имён для ботов: BotMgr выбирает случайное имя при создании бота.
-- При желании список можно расширить, не меняя код.

DROP TABLE IF EXISTS `mod_ai_bots_names`;
CREATE TABLE `mod_ai_bots_names` (
  `id`      INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `name`    VARCHAR(12) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `idx_name` (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Имена для AI-ботов (mod-ai-bots)';

INSERT INTO `mod_ai_bots_names` (`name`) VALUES
  ('Botwarrior'), ('Botmage'), ('Bothunter'), ('Botrogue'), ('Botpriest'),
  ('Botpaladin'), ('Botshaman'), ('Botwarlock'), ('Botdruid'),
  ('Aibo'), ('Bit'), ('Botnik'), ('Botar'), ('Botwen'),
  ('Thalgrim'), ('Maevyn'), ('Korgath'), ('Sylvara'), ('Dornhammer'),
  ('Veliska'), ('Ranthor'), ('Ellowyn'), ('Brakthar'), ('Mirawen'),
  ('Grimshaw'), ('Naylith'), ('Ordwin'), ('Faelira'), ('Zugthrak'),
  ('Liraelle'), ('Kaedric'), ('Morgraine'), ('Sylthara'), ('Bramm'),
  ('Ashvane'), ('Korvan'), ('Illysia'), ('Drakhmar'), ('Wrenna'),
  ('Thalor'), ('Vexira'), ('Grommash'), ('Neryssa'), ('Balgor'),
  ('Caelwyn'), ('Zarakh'), ('Fenris'), ('Malyna'), ('Urthok');

-- Настройки модуля, дублирующие conf/mod_ai_bots.conf.dist (справочно).
DROP TABLE IF EXISTS `mod_ai_bots_settings`;
CREATE TABLE `mod_ai_bots_settings` (
  `setting` VARCHAR(64) NOT NULL,
  `value`   VARCHAR(255) NOT NULL,
  PRIMARY KEY (`setting`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Справочные настройки mod-ai-bots';

-- Точки спавна автономных ботов (режим AiBots.WorldBots.SpawnMode = 0).
-- mapId: 0 = Восточные Королевства, 1 = Калимдор.
-- Точки можно добавлять/менять без пересборки ядра.
DROP TABLE IF EXISTS `mod_ai_bots_locations`;
CREATE TABLE `mod_ai_bots_locations` (
  `id`     INT UNSIGNED NOT NULL AUTO_INCREMENT,
  `mapId`  SMALLINT UNSIGNED NOT NULL DEFAULT 0,
  `x`      FLOAT NOT NULL,
  `y`      FLOAT NOT NULL,
  `z`      FLOAT NOT NULL,
  `o`      FLOAT NOT NULL DEFAULT 0,
  `comment` VARCHAR(64) NOT NULL DEFAULT '',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Точки спавна автономных AI-ботов';

INSERT INTO `mod_ai_bots_locations` (`mapId`, `x`, `y`, `z`, `o`, `comment`) VALUES
  -- Восточные Королевства (mapId = 0)
  (0, -8913.0,  133.0,  87.0, 3.7, 'Elwynn Forest (Northshire)'),
  (0, -9432.0,   66.0,  57.0, 3.7, 'Elwynn Forest'),
  (0, -9165.0,  -218.0, 71.0, 5.9, 'Elwynn Forest (south)'),
  (0, -6240.0,  331.0,  89.0, 4.9, 'Stormwind outskirts'),
  (0, -8928.0, -111.0, 81.0, 1.5, 'Elwynn/Goldshire road'),
  (0, -10520.0, -1182.0, 27.0, 4.5, 'Duskwood (Raven Hill)'),
  (0, -10326.0,  -657.0, 39.0, 5.6, 'Duskwood (Darkshire)'),
  (0,  -9848.0,   913.0, 52.0, 2.3, 'Westfall'),
  (0, -11330.0,  1317.0, 40.0, 1.2, 'Westfall (Sentinel Hill)'),
  (0, -2917.0, -394.0,  55.0, 4.2, 'Dun Morogh'),
  (0, -5380.0, -2950.0, 320.0, 3.1, 'Ironforge approaches'),
  (0, -5583.0, -654.0, 411.0, 3.6, 'Dun Morogh (Kharanos)'),
  (0, -4794.0, -1103.0, 492.0, 4.8, 'Loch Modan'),
  (0, -6182.0, -1808.0, 118.0, 5.3, 'Wetlands'),
  (0,  2571.0, -858.0, 1080.0, 4.9, 'Eastern Plaguelands'),
  (0,  1933.0, -2127.0, 90.0, 5.2, 'Western Plaguelands'),
  (0,  1247.0, -2657.0, 59.0, 5.8, 'Silverpine Forest'),
  (0,  2226.0,  -468.0, 56.0, 5.9, 'Tirisfal Glades (Brill)'),
  (0,  1846.0,  1615.0, 92.0, 2.4, 'Tirisfal (Deathknell)'),
  (0,  1592.0,  238.0, -52.0, 1.4, 'Undercity outskirts'),
  (0, -2594.0,  -563.0, -6.0, 0.7, 'Wetlands (Menethil Harbor)'),
  -- Калимдор (mapId = 1)
  (1,  -618.0, -4251.0, 38.0, 1.5, 'Durotar (Valley of Trials)'),
  (1,  1323.0, -4344.0, 30.0, 5.2, 'Durotar'),
  (1,  1670.0, -4290.0, 61.0, 2.0, 'Orgrimmar outskirts'),
  (1,   225.0, -4726.0, 47.0, 4.5, 'Durotar south'),
  (1,   342.0, -4690.0, 25.0, 0.9, 'Durotar (Razor Hill)'),
  (1, -1044.0, -3424.0, 26.0, 1.8, 'Northern Barrens'),
  (1,  -712.0, -3787.0, 9.0, 5.1, 'Northern Barrens (Crossroads)'),
  (1,  -383.0, -2604.0, 96.0, 3.3, 'Stonetalon Mountains'),
  (1,  1446.0, -1729.0, 55.0, 4.4, 'Ashenvale (Astranaar)'),
  (1,  -4378.0, -3267.0, 34.0, 2.7, 'Mulgore (Bloodhoof Village)'),
  (1,  -2244.0, -407.0, -9.0, 4.7, 'Mulgore (Thunder Bluff road)'),
  (1,  9657.0,  952.0, 1290.0, 5.0, 'Ghostlands');
