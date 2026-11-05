# mod-ai-bots

**[Русский](#русский) | [English](#english)**

---

<a id="русский"></a>

Модуль AI-ботов для [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) (3.3.5a).

Боты — это полноценные персонажи-игроки, которые живут на сервере: следуют за владельцем, сражаются с мобами, дерутся между собой (Альянс ↔ Орда), используют заклинания по классу, лечатся, баффуются, отступают при низком здоровье и общаются в чате — у каждого бота свой характер, внешность и экипировка.

## Возможности

### Боты игрока
- Команды `.bot add / remove / list / follow` (доступны всем игрокам);
- Бот следует за владельцем и участвует в бою рядом с ним;
- Автоспавн при входе игрока в мир (опционально);
- Уровень бота может синхронизироваться с уровнем владельца.

### Автономные боты в мире
- Спавнятся сами при старте сервера в локациях (3 режима выбора точек);
- Сами находят и атакуют мобов поблизости;
- PvP: атакуют ботов враждебной фракции;
- Используют заклинания своего класса;
- Лечатся и баффуются;
- Отступают при низком HP;
- Респавн после смерти через настраиваемое время;
- Пишут в чат на английском (Say/Yell).

### Индивидуальность ботов
- **Личности**: 5 характеров (Aggressive, Calm, Jester, Grump, Hero) — у каждого свои реплики для боя, побед, idle, отступления и лечения. Характер закреплён за ботом навсегда (по guid);
- **Внешность**: случайные skin, face, причёска, цвет волос, борода/усы при создании;
- **Экипировка**: наборы брони plate / mail / leather / cloth и оружие по классу, надеваются автоматически при входе в мир;
- **Уникальные имена**: из SQL-таблицы, процедурный слоговый генератор (~21 000 комбинаций) или смешанный режим.

## Установка

### 1. Клонирование модуля

```bash
cd azerothcore-wotlk/modules
git clone <url-репозитория> mod-ai-bots
```

### 2. Сборка

Модуль подхватывается стандартным механизмом AC-modules при обычной пересборке ядра:

```bash
cd azerothcore-wotlk
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/ac-server
make -j$(nproc)
make install
```

### 3. Применение SQL

Выполните скрипт в базу **world**:

```bash
mysql -u root -p acore_world < modules/mod-ai-bots/data/sql/db-world/mod_ai_bots.sql
```

Скрипт создаёт таблицы:
- `mod_ai_bots_names` — имена ботов;
- `mod_ai_bots_locations` — точки спавна автономных ботов;
- `mod_ai_bots_settings` — справочные настройки.

> Если скрипт уже применялся ранее и вы обновляете модуль — удалите старые данные таблиц перед повторным применением INSERT-блоков.

### 4. Конфигурация

Скопируйте конфиг в папку сервера:

```bash
cp modules/mod-ai-bots/conf/mod_ai_bots.conf.dist $HOME/ac-server/etc/mod_ai_bots.conf
```

��тредактируйте параметры под ваш сервер (см. ниже).

## Настройка (mod_ai_bots.conf)

### Основное

| Опция | По умолчанию | Описание |
|---|---|---|
| `AiBots.Enable` | 0 | Включение модуля |
| `AiBots.MaxBotsPerPlayer` | 1 | Лимит ботов на игрока |
| `AiBots.AutoSpawn` | 0 | Автосоздание бота при логине |
| `AiBots.AutoSpawnCount` | 1 | Сколько ботов создавать при логине |
| `AiBots.UpdateInterval` | 500 | Интервал тика AI, мс |
| `AiBots.EnableCombat` | 1 | Разрешить ботам бой |
| `AiBots.FollowDistance` | 4.0 | Дистанция следования, ярды |
| `AiBots.CommandsForAll` | 1 | Команды доступны всем игрокам |
| `AiBots.DefaultRaceClass` | "1:1" | Раса:класс бота по умолчанию |

### Автономные боты (`AiBots.WorldBots.*`)

| Опция | По умолчанию | Описание |
|---|---|---|
| `Enable` | 1 | Включить автономных ботов |
| `Count` | 10 | Количество ботов в мире |
| `SpawnMode` | 0 | 0 = SQL-таблица, 1 = рядом с онлайн-игроками, 2 = случайные стартовые зоны |
| `RespawnSeconds` | 60 | Респавн после смерти, сек |
| `Level` | 5 | Стартовый уровень автономных ботов |
| `UseSpells` | 1 | Использовать заклинания по классу |

### Чат ботов (`AiBots.WorldBots.Chat.*`)

| Опция | По умолчанию | Описание |
|---|---|---|
| `Enable` | 1 | Разрешить ботам писать в чат |
| `BattleCryChance` | 30 | Шанс боевого клича, % |
| `IdleChance` | 2 | Шанс idle-фразы (раз в 10 сек), % |

### Тактика (`AiBots.Tactics.*`)

| Опция | По умолчанию | Описание |
|---|---|---|
| `FleeHealthPct` | 20 | HP < этого значения → отступление (0 = выкл) |
| `HealThresholdPct` | 50 | HP < этого значения → лечение (0 = выкл) |
| `UseBuffs` | 1 | Баффать себя и союзников перед боем |

### Прочее

| Опция | По умолчанию | Описание |
|---|---|---|
| `AiBots.SyncOwnerLevel` | 0 | > 0: уровень бота игрока = уровень владельца |
| `AiBots.Names.Mode` | 2 | 0 = SQL-таблица, 1 = генератор, 2 = смешанный |

## Команды

| Команда | Описание |
|---|---|
| `.bot add [race:class]` | Создать бота (например `.bot add 7:8` — гном-маг) |
| `.bot remove <имя>` | Удалить бота по имени |
| `.bot remove` (без имени) | Удалить всех своих ботов |
| `.bot list` | Список своих ботов |
| `.bot follow` | Вернуть ботов к следованию за вами |

Расы: 1 = Human, 2 = Orc, 3 = Dwarf, 4 = Night Elf, 5 = Undead, 6 = Tauren, 7 = Gnome, 8 = Troll, 10 = Blood Elf, 11 = Draenei.

Классы: 1 = Warrior, 2 = Paladin, 3 = Hunter, 4 = Rogue, 5 = Priest, 7 = Shaman, 8 = Mage, 9 = Warlock, 11 = Druid.

## Расширение через БД

### Имена ботов (`mod_ai_bots_names`)

```sql
INSERT INTO mod_ai_bots_names (name) VALUES ('МоёИмя');
```

Подхватывается после рестарта сервера. Работает в режимах имён 0 и 2.

### Точки спавна (`mod_ai_bots_locations`)

```sql
INSERT INTO mod_ai_bots_locations (mapId, x, y, z, o, comment)
VALUES (0, -8913.0, 133.0, 87.0, 3.7, 'Моя локация');
```

Используется при `AiBots.WorldBots.SpawnMode = 0`. Координаты можно взять в игре командой `.gps`. Подхватывается после рестарта.

## Структура проекта

```
mod-ai-bots/
├── CMakeLists.txt                  # сборка (AC-modules)
├── conf/
│   └── mod_ai_bots.conf.dist       # конфигурация
├── data/sql/db-world/
│   └── mod_ai_bots.sql             # таблицы world DB
└── src/
    ├── AiBotsScripts.cpp           # команды .bot, хуки, тикер
    ├── BotMgr.h/.cpp               # менеджер ботов (создание, вход в мир, удаление)
    ├── BotAI.h/.cpp                # поведение: следование, бой, спеллы, тактика, чат
    ├── WorldBots.h/.cpp            # автономные боты: спавн, респавн
    ├── BotSpells.h/.cpp            # способности и баффы по классам
    ├── BotPersonality.h/.cpp       # личности и реплики
    ├── BotGear.h/.cpp              # внешность и экипировка
    ├── BotNameGen.h/.cpp           # генератор имён
    ├── Config_Bots.h/.cpp          # чтение конфига
    └── Loader/Loader.h             # точка загрузки скриптов
```

## Как это работает

1. **Создание бота** — модуль вставляет запись персонажа в базу `characters` (со случайной внешностью), затем через несколько секунд бот входит в мир через штатный механизм загрузки игроков;
2. **Поведение** — тикер `WorldScript::OnUpdate` каждые `AiBots.UpdateInterval` мс обновляет AI ботов: следование, поиск целей (мобы 30 ярдов, враждебные боты 40 ярдов), атака (спелл → автоатака), лечение/баффы/отступление;
3. **Чат** — реплики выбираются из личности бота (детерминированной по guid) и произносятся через Say/Yell — их видят окружающие игроки;
4. **Смерть** — автономный бот возрождается через `RespawnSeconds`; при логауте владельца его боты сохраняются и покидают мир.

## Примечания

- Модуль рассчитан на ветку AzerothCore 3.3.5a; API ядра периодически меняется — при ошибках компиляции сверьте сигнатуры вызовов с вашей версией;
- Боты хранятся как обычные персонажи в базе `characters` — удаление модуля не ломает базу (ботов можно удалить командой `.bot remove` или через БД);
- Список личностей и реплик — в `src/BotPersonality.cpp`, легко расширяется.

## Лицензия

GPL-2.0 (как AzerothCore).

---

<a id="english"></a>

# mod-ai-bots (English)

AI bots module for [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) (3.3.5a).

Bots are fully-fledged player characters that live on your server: they follow their owner, fight mobs, battle each other (Alliance ↔ Horde), cast class spells, heal themselves, buff up, flee at low health, and chat — each bot has its own personality, appearance and gear.

## Features

### Player bots
- `.bot add / remove / list / follow` commands (available to all players);
- The bot follows its owner and fights alongside them;
- Auto-spawn on player login (optional);
- Bot level can be synced with the owner's level.

### Autonomous world bots
- Spawn automatically at server startup in world locations (3 spawn-point modes);
- Find and attack nearby mobs on their own;
- PvP: attack bots of the opposing faction;
- Cast their class spells;
- Heal and buff themselves;
- Flee at low HP;
- Respawn after death after a configurable delay;
- Chat in English (Say/Yell).

### Bot individuality
- **Personalities**: 5 characters (Aggressive, Calm, Jester, Grump, Hero) — each with its own battle cries, victory lines, idle chatter, retreat and healing phrases. A bot's personality is permanent (bound to its guid);
- **Appearance**: random skin, face, hairstyle, hair color and facial features at creation;
- **Gear**: plate / mail / leather / cloth armor sets and weapons by class, equipped automatically on entering the world;
- **Unique names**: from an SQL table, a procedural syllable generator (~21,000 combinations), or a mixed mode.

## Installation

### 1. Clone the module

```bash
cd azerothcore-wotlk/modules
git clone <repository-url> mod-ai-bots
```

### 2. Build

The module is picked up by the standard AC-modules mechanism during a normal core rebuild:

```bash
cd azerothcore-wotlk
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/ac-server
make -j$(nproc)
make install
```

### 3. Apply SQL

Run the script against the **world** database:

```bash
mysql -u root -p acore_world < modules/mod-ai-bots/data/sql/db-world/mod_ai_bots.sql
```

The script creates the following tables:
- `mod_ai_bots_names` — bot names;
- `mod_ai_bots_locations` — spawn points for autonomous bots;
- `mod_ai_bots_settings` — reference settings.

> If the script was applied before and you are upgrading the module, clear the old table data before re-applying the INSERT blocks.

### 4. Configuration

Copy the config to your server directory:

```bash
cp modules/mod-ai-bots/conf/mod_ai_bots.conf.dist $HOME/ac-server/etc/mod_ai_bots.conf
```

Edit the options to fit your server (see below).

## Configuration (mod_ai_bots.conf)

### General

| Option | Default | Description |
|---|---|---|
| `AiBots.Enable` | 0 | Enable the module |
| `AiBots.MaxBotsPerPlayer` | 1 | Bot limit per player |
| `AiBots.AutoSpawn` | 0 | Auto-create a bot on login |
| `AiBots.AutoSpawnCount` | 1 | How many bots to create on login |
| `AiBots.UpdateInterval` | 500 | AI tick interval, ms |
| `AiBots.EnableCombat` | 1 | Allow bots to fight |
| `AiBots.FollowDistance` | 4.0 | Follow distance, yards |
| `AiBots.CommandsForAll` | 1 | Commands available to all players |
| `AiBots.DefaultRaceClass` | "1:1" | Default bot race:class |

### World bots (`AiBots.WorldBots.*`)

| Option | Default | Description |
|---|---|---|
| `Enable` | 1 | Enable autonomous bots |
| `Count` | 10 | Number of bots in the world |
| `SpawnMode` | 0 | 0 = SQL table, 1 = near online players, 2 = random starting zones |
| `RespawnSeconds` | 60 | Respawn delay after death, sec |
| `Level` | 5 | Starting level of autonomous bots |
| `UseSpells` | 1 | Use class spells |

### Bot chat (`AiBots.WorldBots.Chat.*`)

| Option | Default | Description |
|---|---|---|
| `Enable` | 1 | Allow bots to chat |
| `BattleCryChance` | 30 | Battle cry chance, % |
| `IdleChance` | 2 | Idle phrase chance (checked every 10 sec), % |

### Tactics (`AiBots.Tactics.*`)

| Option | Default | Description |
|---|---|---|
| `FleeHealthPct` | 20 | HP below this → flee (0 = disabled) |
| `HealThresholdPct` | 50 | HP below this → heal (0 = disabled) |
| `UseBuffs` | 1 | Buff self and allies before combat |

### Miscellaneous

| Option | Default | Description |
|---|---|---|
| `AiBots.SyncOwnerLevel` | 0 | > 0: player bot level = owner's level |
| `AiBots.Names.Mode` | 2 | 0 = SQL table, 1 = generator, 2 = mixed |

## Commands

| Command | Description |
|---|---|
| `.bot add [race:class]` | Create a bot (e.g. `.bot add 7:8` — gnome mage) |
| `.bot remove <name>` | Remove a bot by name |
| `.bot remove` (no name) | Remove all of your bots |
| `.bot list` | List your bots |
| `.bot follow` | Make your bots follow you again |

Races: 1 = Human, 2 = Orc, 3 = Dwarf, 4 = Night Elf, 5 = Undead, 6 = Tauren, 7 = Gnome, 8 = Troll, 10 = Blood Elf, 11 = Draenei.

Classes: 1 = Warrior, 2 = Paladin, 3 = Hunter, 4 = Rogue, 5 = Priest, 7 = Shaman, 8 = Mage, 9 = Warlock, 11 = Druid.

## Database extensions

### Bot names (`mod_ai_bots_names`)

```sql
INSERT INTO mod_ai_bots_names (name) VALUES ('MyName');
```

Picked up after a server restart. Works in name modes 0 and 2.

### Spawn points (`mod_ai_bots_locations`)

```sql
INSERT INTO mod_ai_bots_locations (mapId, x, y, z, o, comment)
VALUES (0, -8913.0, 133.0, 87.0, 3.7, 'My location');
```

Used when `AiBots.WorldBots.SpawnMode = 0`. Coordinates can be obtained in-game with `.gps`. Picked up after a restart.

## Project structure

```
mod-ai-bots/
├── CMakeLists.txt                  # build script (AC-modules)
├── conf/
│   └── mod_ai_bots.conf.dist       # configuration
├── data/sql/db-world/
│   └── mod_ai_bots.sql             # world DB tables
└── src/
    ├── AiBotsScripts.cpp           # .bot commands, hooks, ticker
    ├── BotMgr.h/.cpp               # bot manager (creation, world entry, removal)
    ├── BotAI.h/.cpp                # behavior: follow, combat, spells, tactics, chat
    ├── WorldBots.h/.cpp            # autonomous bots: spawn, respawn
    ├── BotSpells.h/.cpp            # class abilities and buffs
    ├── BotPersonality.h/.cpp       # personalities and chat lines
    ├── BotGear.h/.cpp              # appearance and equipment
    ├── BotNameGen.h/.cpp           # name generator
    ├── Config_Bots.h/.cpp          # config reader
    └── Loader/Loader.h             # script loader entry point
```

## How it works

1. **Bot creation** — the module inserts a character row into the `characters` database (with a random appearance), then after a few seconds the bot enters the world through the standard player-loading pipeline;
2. **Behavior** — the `WorldScript::OnUpdate` ticker refreshes bot AI every `AiBots.UpdateInterval` ms: following, target search (mobs within 30 yards, hostile bots within 40 yards), attack (spell → auto-attack), healing/buffs/retreat;
3. **Chat** — lines are picked from the bot's personality (deterministic, bound to its guid) and spoken via Say/Yell — visible to nearby players;
4. **Death** — an autonomous bot respawns after `RespawnSeconds`; when the owner logs out, their bots are saved and leave the world.

## Notes

- The module targets the AzerothCore 3.3.5a branch; the core API changes over time — if compilation fails, verify call signatures against your core version;
- Bots are stored as regular characters in the `characters` database — removing the module does not break anything (bots can be removed via `.bot remove` or the database);
- The personality and chat line lists live in `src/BotPersonality.cpp` and are easy to extend.

## License

GPL-2.0 (same as AzerothCore).
