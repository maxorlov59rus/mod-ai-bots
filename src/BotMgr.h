/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotMgr.h
 *
 * Менеджер AI-ботов: создание, вход в мир, удаление, хранение.
 */

#ifndef AI_BOTS_BOT_MGR_H
#define AI_BOTS_BOT_MGR_H

#include "Define.h"

#include <cstdint>
#include <map>
#include <set>
#include <vector>

class Player;
class WorldSession;
class LoginQueryHolder;

struct BotData
{
    ObjectGuid::LowType guid;
    ObjectGuid::LowType ownerGuid;
    std::string         name;
    uint8               race;
    uint8               cls;
    uint8               level;
    bool                inWorld;
};

class BotMgr
{
public:
    static BotMgr* instance();

    // Создать бота для игрока (по умолчанию — случайные раса/класс из конфига).
    bool CreateBotForPlayer(Player* owner, uint8 race = 0, uint8 cls = 0);

    // Удалить бота (выйти из мира + удалить персонажа из БД).
    bool RemoveBot(Player* owner, std::string const& botName);

    // Удалить всех ботов игрока.
    void RemoveAllBots(Player* owner);

    // Вход бота в мир после завершения асинхронной загрузки из БД.
    void HandleBotLogin(ObjectGuid::LowType botGuid);

    // Выход бота из мира (при выходе владельца или завершении работы).
    void LogoutBot(ObjectGuid::LowType botGuid);

    // Ежесекундное обновление: вхождение в мир отложенных ботов, чистка.
    void Update(uint32 diff);

    // Поиск
    Player* GetBotPlayer(ObjectGuid::LowType botGuid) const;
    std::vector<BotData> const& GetBotsOfOwner(ObjectGuid::LowType ownerGuid) const;
    std::string const& GetBotName(ObjectGuid::LowType botGuid) const;

    // Вызывается при выходе игрока из мира — удаляет его ботов.
    void HandlePlayerLogout(Player* player);

    // При загрузке игрока в мир — автоспавн, если включён.
    void HandlePlayerLogin(Player* player);

    // Восстановление ботов владельца из БД после рестарта сервера.
    void LoadOwnedBots(ObjectGuid::LowType ownerGuid);

    uint32 GetBotCountForPlayer(ObjectGuid::LowType ownerGuid) const;

    // Все боты (для социальных подсистем).
    std::map<ObjectGuid::LowType, BotData> const& GetAllBots() const { return _allBots; }

    // Вход бота в мир: создаёт WorldSession без сокета и грузит персонажа
    // через штатный LoginQueryHolder-пайплайн ядра (бот появляется в /who,
    // в онлайне сервера и characters.online = 1).
    bool EnterBotWorld(ObjectGuid::LowType botGuid);

    // Уникальное имя бота: случайное из кеша (mod_ai_bots_names), не занятое
    // другими ботами/персонажами; при исчерпании — числовой суффикс.
    std::string GenerateUniqueBotName() const;

private:
    BotMgr() = default;

    // Загружает имена из таблицы mod_ai_bots_names (world DB) в кеш.
    // Если таблица пуста/отсутствует — заполняет кеш встроенным списком.
    void LoadBotNames();
    bool IsNameTaken(std::string const& name) const;
    bool EnterWorld(ObjectGuid::LowType botGuid, ObjectGuid::LowType ownerGuid);
    void ScheduleEnterWorld(ObjectGuid::LowType botGuid, ObjectGuid::LowType ownerGuid);

    // Кеш имён из mod_ai_bots_names; заполняется лениво при первом создании бота.
    mutable std::vector<std::string> _botNames;
    mutable bool _botNamesLoaded = false;

    std::map<ObjectGuid::LowType, std::vector<BotData>> _botsByOwner;
    std::map<ObjectGuid::LowType, BotData> _allBots;

    // Guid владельцев, чьи боты уже восстановлены из БД (защита от повторной загрузки).
    std::set<ObjectGuid::LowType> _ownersLoaded;

    struct PendingLogin
    {
        ObjectGuid::LowType botGuid;
        ObjectGuid::LowType ownerGuid;
        uint32 remainingMs;
    };
    std::vector<PendingLogin> _pendingLogins;

    uint32 _updateTimer = 0;
};

#define sBotMgr BotMgr::instance()

#endif // AI_BOTS_BOT_MGR_H
