# Патч ядра AzerothCore для ботов без сокета (bot sessions)

Модуль создаёт ботам `WorldSession` без сетевого сокета — так боты появляются
в онлайне сервера (`/who`, счётчик онлайна, `characters.online = 1`).
Для корректной работы таких сессий нужно внести небольшие изменения в ядро
AzerothCore 3.3.5a (ветка master).

## 1. WorldSession: флаг бот-сессии

**Файл: `src/server/game/Server/WorldSession.h`**

В секцию private добавьте поле:

```cpp
private:
    bool _botSession = false;
```

В секцию public добавьте методы:

```cpp
public:
    void SetBotSession(bool enabled) { _botSession = enabled; }
    bool IsBotSession() const { return _botSession; }
```

## 2. World: пропуск сетевой обработки для бот-сессий

**Файл: `src/server/game/Server/World/World.cpp`**

В `World::UpdateSessions` найдите цикл, где обновляются сессии
(ветка `case STATUS_LOGGEDIN` или основной `Update(diff, updater)`),
и пропустите сетевые операции для ботов:

```cpp
// В месте обработки UpdatePacketFilter / сокета сессии:
if (session->IsBotSession())
    continue; // у бот-сессий нет сокета — сетевой ввод не обрабатываем
```

Конкретное место зависит от версии ядра: ищите в `World::UpdateSessions()`
блок с `WorldSessionFilterUpdater` и оберните вызов `session->Update(...)`
так, чтобы для бот-сессий выполнялся только логический апдейт:

```cpp
if (session->IsBotSession())
    session->Update(diff, Acore::PacketFilter()); // без сетевых пакетов
else
    session->Update(diff, updater);
```

## 3. Защита от отправки пакетов ботам (опционально)

**Файл: `src/server/game/Server/WorldSession.cpp`**

В `WorldSession::SendPacket` можно добавить ранний выход для бот-сессий
(экономия CPU на подготовке пакетов), но осторожно: часть пакетов влияет
на логику ядра. Безопасный вариант — ничего не менять: пустой сокет
корректно отбрасывает пакеты.

```cpp
// ОПЦИОНАЛЬНО (может ломать часть логики — тестируйте):
// if (IsBotSession())
//     return;
```

## 4. Как применить

1. Внесите правки из шагов 1–2 в исходники ядра;
2. Пересоберите ядро:
   ```bash
   cd azerothcore-wotlk/build
   cmake ..
   make -j$(nproc) && make install
   ```

## 5. Аккаунт ботов

Рекомендуется завести в базе `auth` отдельный аккаунт и указать его в конфиге:

```sql
INSERT INTO account (id, username, sha_pass_hash, expansion)
VALUES (777, 'AIBOTS', '', 2);
```

```ini
# mod_ai_bots.conf
AiBots.AccountId = 777
```

## Примечания

- Альтернативный путь: использовать готовые патчи ядра из проектов
  playerbots (mod-playerbots) — они уже содержат поддержку bot-сессий
  и совместимы по идее с нашим подходом;
- Без патча шагов 1–2 модуль продолжит собираться, но сессии ботов могут
  некорректно завершаться при рестарте мира. Функционал онлайн-листа
  требует именно этих двух правок.
