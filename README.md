# Social Network (Социальная Сеть)

**Язык / Language:** [Русский](#команды) | [English](#commands)

Fun-модуль для AzerothCore: внутреигровая социальная сеть — профили игроков, стена записей, лайки, комментарии, личные сообщения, подписки, топ по лайкам и **NPC-терминал с меню в клиенте**.

## Команды

| Команда | Описание |
|---|---|
| `.social profile [имя]` | Профиль: уровень, время в игре, гильдия, статус, лайки, подписчики/подписки. |
| `.social status <текст>` | Установить статус в профиле. |
| `.social post <текст>` | Опубликовать запись на своей стене (подписчики онлайн получают уведомление). |
| `.social wall [имя]` | Последние записи со стены (своей или чужой). |
| `.social delete <id>` | Удалить свою запись (и её лайки/комментарии). |
| `.social like <имя>` | Лайкнуть последнюю запись игрока (1 раз на запись, нельзя себе). |
| `.social comment <id> <текст>` | Оставить комментарий к записи. |
| `.social comments <id>` | Прочитать комментарии записи. |
| `.social msg <имя> <текст>` | Личное сообщение игроку (доставляется сразу, если он онлайн). |
| `.social inbox` | Входящие сообщения. |
| `.social msgdel <id>` | Удалить сообщение из входящих. |
| `.social follow <имя>` | Подписаться на игрока. |
| `.social unfollow <имя>` | Отписаться от игрока. |
| `.social friend <имя>` | Заявка в друзья (взаимная: принимающий видит уведомление). |
| `.social friendaccept <имя>` | Принять заявку в друзья (дружба появляется у обоих). |
| `.social frienddecline <имя>` | Отклонить заявку. |
| `.social friends` | Список друзей. |
| `.social requests` | Входящие заявки в друзья (кто вас пригласил). |
| `.social frienddel <имя>` | Удалить из друзей (у обоих). |
| `.social enemy <имя>` | Добавить во враги (нельзя друга; лимит 10). |
| `.social enemies` | Список врагов. |
| `.social enemydel <имя>` | Убрать из врагов. |
| `.social guild` | Карточка вашей гильдии: гилдмастер, число участников, список и онлайн. |
| `.social top` | Топ-10 игроков по лайкам. |
| `.social deleteany <id>` | Удалить любую запись (только для RBAC 1101 — модерация). |

## NPC-терминал (интерфейс в клиенте)

В мире стоит **Социальная Сеть — терминал общения** (NPC 911100, Stormwind у Аукционного Дома). Клик по нему открывает **gossip-меню в клиенте**:

- **Мой профиль** — сводка профиля.
- **Моя стена** — последние записи.
- **Входящие** — личные сообщения.
- **Топ по лайкам**.
- **Подсказка** — список команд.

Меню локализовано (RU/EN по клиенту игрока). Посты/комментарии/сообщения вводятся через чат-команды — штатное окно ввода gossip (player-choice box) в AzerothCore-модулях не переносимо, поэтому записи пишутся так: открыли терминал → «Подсказка» → `.social post Привет!`.

Позицию NPC можно изменить в `data/sql/db-world/social_network_world.sql` (или командой `.npc move`).

## Аддон-компаньон (интерфейс в клиенте)

Репозиторий включает **аддон `addon/SocialNetwork/`** — окно Социальной Сети внутри клиента 3.3.5:

- Кнопки: **Профиль / Стена / Входящие / Топ** — просмотр без команд.
- **Поле ввода + «Отправить»** — публикация записи на стену прямо из окна.
- Слэш-команды: `/socn` (открыть окно), `/socn wall <имя>` (быстрый запрос в чат), также работает `/соцсеть`.
- Полная локализация интерфейса (RU/EN через TOC-метаданные).

### Как работает

Аддон общается с сервером через **штатный addon-канал ядра AzerothCore** (`AddonChannelCommandHandler`, протокол `AzerothCore	`), поэтому **правки ядра не нужны** — сервер лишь должен иметь модуль `mod-social-network` с командами `.social ...`.

### Установка аддона

1. Скопируйте папку `addon/SocialNetwork` в клиент: `Interface/AddOns/SocialNetwork`.
2. Перезапустите клиент, включите аддон на экране выбора персонажа.
3. Откройте окно: `/socn` или `/соцсеть` (либо через NPC-терминал).

## Смайлики

В записях, комментариях и сообщениях текстовые смайлы автоматически заменяются на **цветные иконки клиента** (через `|T...|t` текстуры — работает в 3.3.5 без аддонов):

| Ввод | Иконка |
|---|---|
| `:)` `:-)` | Звезда-улыбка |
| `:D` `:-D` | Большой смех (гемы) |
| `;)` `;-)` | Подмигивание (глаз) |
| `:P` `:-P` | Язык (еда) |
| `:(` `:-(` | Грусть (тёмный чарм) |
| `>:(` | Злость (красный гем) |
| `<3` | Сердце (розы) |
| `</3` | Разбитое сердце (валентинка) |
| `o_O` `O_o` | Удивление (глаз) |
| `:O` | Шок (гем) |
| `Zzz` `zzz` | Сон (кровать) |
| `:*` | Поцелуй (гем) |
| `\o/` | Ура! (игрушка) |

Технически: замена выполняется на сервере **после** очистки от цветовых кодов (`|`) и проверки длины — лимит считается по набранным символам, а не по байтам иконок. Длинные последовательности проверяются первыми (`:-)` раньше `:)`), чтобы не было ложных срабатываний.

## Локализация

**Полная поддержка английского и русского** (а также frFR, deDE, esES, esMX, zhCN, zhTW, koKR):

- EN — дефолт (`module_string`), RU — перевод (`module_string_locale`).
- Игрок получает сообщения на локали своего клиента автоматически.
- Если SQL строк не применён — встроенные английские fallback-строки в коде.
- Весь пользовательский текст — UTF-8: кириллица в статусах, постах, комментариях и сообщениях работает корректно (валидация `Utf8toWStr`, лимиты в символах, регистронезависимый поиск кириллических имён).

## База данных

Данные игроков хранятся в **acore_characters** (привязаны к персонажам):

| Таблица | Назначение |
|---|---|
| `socn_profiles` | guid, статус |
| `socn_posts` | записи стены (id, автор, текст, время) |
| `socn_post_likes` | лайки (postId, voterGuid) |
| `socn_comments` | комментарии (id, postId, автор, текст) |
| `socn_messages` | личные сообщения (id, отправитель, получатель, текст) |
| `socn_friends` | друзья (взаимные пары owner/friend) |
| `socn_friend_requests` | заявки в друзья (from/to) |
| `socn_enemies` | враги (owner/enemy) |
| `socn_follows` | подписки (followerGuid, targetGuid) |

**Друзья** — взаимные: заявка → уведомление получателю (онлайн) → `friendaccept` создаёт дружбу у обоих, `frienddel` удаляет у обоих. `.social requests` показывает входящие заявки. **В топе лайков** имена ваших друзей помечаются меткой `[Друг]` (персонально для каждого смотрящего). **Враги** — односторонний список; нельзя добавить себя или друга. **Гильдия** — данные берутся из штатных таблиц `guild`/`guild_member` (интеграция с ядром), показываются гилдмастер, состав и онлайн.

RBAC добавляется в **acore_auth** (permissions 1100, 1101), строки локализации и NPC-терминал — в **acore_world**.

## Конфиг

`conf/mod-social-network.conf.dist`:

| Опция | По умолчанию | Описание |
|---|---|---|
| `SocialNetwork.MaxPosts` | `10` | Максимум записей на стене игрока. |
| `SocialNetwork.MaxStatusLength` | `60` | Максимум длина статуса (символов). |
| `SocialNetwork.MaxPostLength` | `255` | Максимум длина записи (символов). |
| `SocialNetwork.WallPageSize` | `5` | Сколько записей показывает `.social wall`. |
| `SocialNetwork.TopSize` | `10` | Размер топа в `.social top`. |
| `SocialNetwork.InboxPageSize` | `10` | Сколько сообщений показывает `.social inbox`. |
| `SocialNetwork.MaxMessageLength` | `255` | Максимум длина сообщения/комментария (символов). |
| `SocialNetwork.MaxComments` | `20` | Максимум комментариев к записи. |

## Установка

1. Склонируйте модуль в `modules/mod-social-network/` и пересоберите worldserver:
   ```bash
   cd azerothcore-wotlk/modules
   git clone https://git.sourcecraft.dev/crazy-voin/mod-social-network.git mod-social-network
   ```
2. Примените SQL из `data/sql/db-characters/social_network.sql`:
   * таблицы — к базе **acore_characters**;
   * блок RBAC — к базе **acore_auth**;
   * блок строк — к базе **acore_world**.
3. Примените `data/sql/db-world/social_network_update.sql` — к базе **acore_world** (новые строки меню/сообщений/комментариев).
4. Примените `data/sql/db-world/social_network_world.sql` — к базе **acore_world** (NPC-терминал 911100).
5. Примените `data/sql/db-world/social_network_friends.sql` — к базам: таблицы → **acore_characters**, строки → **acore_world**.
6. Примените `data/sql/db-world/social_network_requests.sql` — к базе **acore_world** (строки 82–84).
7. Опционально скопируйте `conf/mod-social-network.conf.dist` → `mod-social-network.conf` и настройте лимиты.
8. Чтобы ограничить `.social deleteany`, уберите permission 1101 из роли Player в `rbac_role_permissions`.

---

# Social Network — English version

Fun module for AzerothCore: an in-game social network — player profiles, a wall of posts, likes, comments, private messages, follows, a like top and an **NPC terminal with an in-client menu**.

## Commands

| Command | Description |
|---|---|
| `.social profile [name]` | Profile: level, play time, guild, status, likes, followers/following. |
| `.social status <text>` | Set your profile status. |
| `.social post <text>` | Publish a post on your wall (online followers get notified). |
| `.social wall [name]` | Latest wall posts (yours or another player's). |
| `.social delete <id>` | Delete your own post (with its likes/comments). |
| `.social like <name>` | Like the player's latest post (once per post, not your own). |
| `.social comment <id> <text>` | Add a comment to a post. |
| `.social comments <id>` | Read a post's comments. |
| `.social msg <name> <text>` | Private message (delivered instantly when the receiver is online). |
| `.social inbox` | Inbox messages. |
| `.social msgdel <id>` | Delete an inbox message. |
| `.social follow <name>` | Follow a player. |
| `.social unfollow <name>` | Unfollow a player. |
| `.social friend <name>` | Friend request (mutual: the receiver gets notified). |
| `.social friendaccept <name>` | Accept a friend request (both sides become friends). |
| `.social frienddecline <name>` | Decline a friend request. |
| `.social friends` | Friends list. |
| `.social requests` | Incoming friend requests (who invited you). |
| `.social frienddel <name>` | Remove from friends (on both sides). |
| `.social enemy <name>` | Add to enemies (friends not allowed; limit 10). |
| `.social enemies` | Enemies list. |
| `.social enemydel <name>` | Remove from enemies. |
| `.social guild` | Your guild card: guild master, member count, list and online. |
| `.social top` | Top 10 players by likes. |
| `.social deleteany <id>` | Delete any post (RBAC 1101 only — moderation). |

## NPC Terminal (in-client interface)

The world contains the **Social Network communication terminal** (NPC 911100, Stormwind near the Auction House). Clicking it opens a **gossip menu in the client**:

- **My profile** — profile summary.
- **My wall** — latest posts.
- **Inbox** — private messages.
- **Top by likes**.
- **Hint** — command list.

The menu is localized (RU/EN by player's client). Posts/comments/messages are entered via chat commands — the stock gossip input box is not portable in AzerothCore modules, so posting works like this: open the terminal → "Hint" → `.social post Hello!`.

The NPC position can be changed in `data/sql/db-world/social_network_world.sql` (or with `.npc move`).

## Addon companion (in-client interface)

The repository includes the **`addon/SocialNetwork/` addon** — a Social Network window inside the 3.3.5 client:

- Buttons: **Profile / Wall / Inbox / Top** — browsing without commands.
- **Input box + "Send"** — publish a wall post right from the window.
- Slash commands: `/socn` (open the window), `/socn wall <name>` (quick chat query); `/соцсеть` also works.
- Full interface localization (RU/EN via TOC metadata).

### How it works

The addon talks to the server through the **stock AzerothCore addon channel** (`AddonChannelCommandHandler`, the `AzerothCore	` protocol), so **no core edits are needed** — the server only needs the `mod-social-network` module with the `.social ...` commands.

### Addon installation

1. Copy the `addon/SocialNetwork` folder into the client: `Interface/AddOns/SocialNetwork`.
2. Restart the client, enable the addon on the character selection screen.
3. Open the window: `/socn` (or via the NPC terminal).

## Smilies

In posts, comments and messages text emoticons are automatically replaced with **colored client icons** (via `|T...|t` textures — works in 3.3.5 without addons):

| Input | Icon |
|---|---|
| `:)` `:-)` | Star smile |
| `:D` `:-D` | Big grin (gems) |
| `;)` `;-)` | Wink (eye) |
| `:P` `:-P` | Tongue (food) |
| `:(` `:-(` | Sad (dark charm) |
| `>:(` | Angry (red gem) |
| `<3` | Heart (roses) |
| `</3` | Broken heart (valentine) |
| `o_O` `O_o` | Surprised (eye) |
| `:O` | Shock (gem) |
| `Zzz` `zzz` | Sleep (bed) |
| `:*` | Kiss (gem) |
| `\o/` | Hooray! (toy) |

Technically: the replacement happens on the server **after** stripping color codes (`|`) and the length check — the limit counts typed characters, not icon bytes. Longer sequences are matched first (`:-)` before `:)`) to avoid false matches.

## Localization

**Full English and Russian support** (plus frFR, deDE, esES, esMX, zhCN, zhTW, koKR):

- EN — default (`module_string`), RU — translation (`module_string_locale`).
- Each player automatically receives messages in their client's locale.
- If the string SQL is not applied — built-in English fallback strings in the code.
- All user text is UTF-8: Cyrillic in statuses, posts, comments and messages works correctly (`Utf8toWStr` validation, character-based limits, case-insensitive search for Cyrillic names).

## Database

Player data is stored in **acore_characters** (bound to characters):

| Table | Purpose |
|---|---|
| `socn_profiles` | guid, status |
| `socn_posts` | wall posts (id, author, text, time) |
| `socn_post_likes` | likes (postId, voterGuid) |
| `socn_comments` | comments (id, postId, author, text) |
| `socn_messages` | private messages (id, sender, receiver, text) |
| `socn_friends` | friends (mutual owner/friend pairs) |
| `socn_friend_requests` | friend requests (from/to) |
| `socn_enemies` | enemies (owner/enemy) |
| `socn_follows` | follows (followerGuid, targetGuid) |

**Friends** are mutual: request → online notification → `friendaccept` creates the friendship on both sides, `frienddel` removes it on both sides. `.social requests` lists incoming requests. **In the like top**, your friends' names are marked with a `[Friend]` tag (per viewer). **Enemies** — one-directional list; you cannot add yourself or a friend. **Guild** — data comes from the stock `guild`/`guild_member` tables (core integration): guild master, roster and online status are shown.

RBAC goes into **acore_auth** (permissions 1100, 1101), localization strings and the NPC terminal — into **acore_world**.

## Configuration

`conf/mod-social-network.conf.dist`:

| Option | Default | Description |
|---|---|---|
| `SocialNetwork.MaxPosts` | `10` | Max wall posts per player. |
| `SocialNetwork.MaxStatusLength` | `60` | Max status length (characters). |
| `SocialNetwork.MaxPostLength` | `255` | Max post length (characters). |
| `SocialNetwork.WallPageSize` | `5` | How many posts `.social wall` shows. |
| `SocialNetwork.TopSize` | `10` | Top size in `.social top`. |
| `SocialNetwork.InboxPageSize` | `10` | How many messages `.social inbox` shows. |
| `SocialNetwork.MaxMessageLength` | `255` | Max message/comment length (characters). |
| `SocialNetwork.MaxComments` | `20` | Max comments per post. |

## Installation

1. Clone the module into `modules/mod-social-network/` and rebuild the worldserver:
   ```bash
   cd azerothcore-wotlk/modules
   git clone https://git.sourcecraft.dev/crazy-voin/mod-social-network.git mod-social-network
   ```
2. Apply the SQL from `data/sql/db-characters/social_network.sql`:
   * tables → to the **acore_characters** database;
   * RBAC block → to the **acore_auth** database;
   * strings block → to the **acore_world** database.
3. Apply `data/sql/db-world/social_network_update.sql` → to **acore_world** (new menu/message/comment strings).
4. Apply `data/sql/db-world/social_network_world.sql` → to **acore_world** (NPC terminal 911100).
5. Apply `data/sql/db-world/social_network_friends.sql` — databases: tables → **acore_characters**, strings → **acore_world**.
6. Apply `data/sql/db-world/social_network_requests.sql` → to **acore_world** (strings 82–84).
7. Optionally copy `conf/mod-social-network.conf.dist` → `mod-social-network.conf` and tune the limits.
8. To restrict `.social deleteany`, remove permission 1101 from the Player role in `rbac_role_permissions`.
