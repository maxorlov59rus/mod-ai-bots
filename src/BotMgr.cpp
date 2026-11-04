/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotMgr.cpp
 *
 * Реализация менеджера ботов.
 *
 * Подход: бот — это реальный персонаж в базе characters, который входит
 * в мир через собственный WorldSession (LoginQueryHolder), как это делают
 * игроки. Поведением управляет BotAI (см. BotAI.h/cpp).
 */

#include "BotMgr.h"
#include "Config_Bots.h"
#include "BotAI.h"
#include "BotNameGen.h"
#include "BotGear.h"

#include "CharacterCache.h"
#include "Chat.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "World.h"
#include "WorldSession.h"

#include <algorithm>
#include <random>

namespace
{
    std::string const BotSessionName = "AI-BOT";
    uint32 const BotAccount = 0; // боты не привязаны к реальным аккаунтам

    bool IsValidRaceClass(uint8 race, uint8 cls)
    {
        // Разрешаем альянс/орда стартовые комбинации без уникальных классов.
        if (!(race == RACE_HUMAN || race == RACE_ORC || race == RACE_DWARF ||
              race == RACE_NIGHTELF || race == RACE_UNDEAD_PLAYER || race == RACE_TAUREN ||
              race == RACE_GNOME || race == RACE_TROLL || race == RACE_BLOODELF ||
              race == RACE_DRAENEI))
            return false;

        if (!(cls == CLASS_WARRIOR || cls == CLASS_PALADIN || cls == CLASS_HUNTER ||
              cls == CLASS_ROGUE || cls == CLASS_PRIEST || cls == CLASS_SHAMAN ||
              cls == CLASS_MAGE || cls == CLASS_WARLOCK || cls == CLASS_DRUID))
            return false;

        // Упрощённая проверка доступности класса для расы.
        switch (cls)
        {
            case CLASS_PALADIN:
                return race == RACE_HUMAN || race == RACE_DWARF || race == RACE_DRAENEI;
            case CLASS_SHAMAN:
                return race == RACE_ORC || race == RACE_TAUREN || race == RACE_TROLL || race == RACE_DRAENEI;
            case CLASS_DRUID:
                return race == RACE_NIGHTELF || race == RACE_TAUREN;
            case CLASS_HUNTER:
                return race != RACE_HUMAN && race != RACE_GNOME;
            case CLASS_MAGE:
                return race != RACE_TAUREN;
            case CLASS_PRIEST:
                return race != RACE_TAUREN && race != RACE_ORC;
            case CLASS_WARLOCK:
                return race == RACE_HUMAN || race == RACE_ORC || race == RACE_GNOME || race == RACE_UNDEAD_PLAYER || race == RACE_BLOODELF;
            case CLASS_ROGUE:
                return race != RACE_DRAENEI && race != RACE_TAUREN;
            default:
                return true;
        }
    }

    void ParseRaceClass(std::string const& value, uint8& race, uint8& cls)
    {
        race = RACE_HUMAN;
        cls = CLASS_WARRIOR;
        auto pos = value.find(':');
        if (pos == std::string::npos)
            return;

        try
        {
            int r = std::stoi(value.substr(0, pos));
            int c = std::stoi(value.substr(pos + 1));
            if (r > 0 && c > 0)
            {
                race = static_cast<uint8>(r);
                cls = static_cast<uint8>(c);
            }
        }
        catch (std::exception const&)
        {
            // Оставляем значения по умолчанию.
        }
    }
}

BotMgr* BotMgr::instance()
{
    static BotMgr instance;
    return &instance;
}

std::string BotMgr::GenerateUniqueBotName() const
{
    // Ленивая загрузка имён из таблицы mod_ai_bots_names (world DB).
    if (!_botNamesLoaded)
    {
        LoadBotNames();
        _botNamesLoaded = true;
    }

    // Fallback, если таблица пуста или БД недоступна.
    if (_botNames.empty())
    {
        static std::vector<std::string> const fallback =
        {
            "BotWarrior", "BotMage", "BotHunter", "BotRogue", "BotPriest",
            "BotPaladin", "BotShaman", "BotWarlock", "BotDruid",
            "Aibo", "Bit", "Botnik", "Botar", "Botwen"
        };
        _botNames = fallback;
    }

    uint8 namesMode = AiBotsConfig::GetNamesMode();

    // Режим 1: чистый генератор — до 25 попыток найти незанятое имя.
    if (namesMode == 1)
    {
        for (int attempt = 0; attempt < 25; ++attempt)
        {
            std::string candidate = AiBotsNameGen::Generate();
            if (!IsNameTaken(candidate))
                return candidate;
        }

        // Крайний случай: генератор с числовым суффиксом.
        std::string base = AiBotsNameGen::Generate();
        for (uint32 suffix = 2; suffix < 100; ++suffix)
        {
            std::string candidate = base.substr(0, 12 - std::to_string(suffix).length()) + std::to_string(suffix);
            if (!IsNameTaken(candidate))
                return candidate;
        }

        return base;
    }

    // Режимы 0 и 2: таблица имён.
    std::vector<std::string> candidates = _botNames;
    std::shuffle(candidates.begin(), candidates.end(), std::mt19937(std::random_device()()));

    for (std::string const& candidate : candidates)
    {
        if (!IsNameTaken(candidate))
            return candidate;
    }

    // Таблица исчерпана.
    if (namesMode == 2)
    {
        // Смешанный режим: переходим на генератор.
        for (int attempt = 0; attempt < 25; ++attempt)
        {
            std::string candidate = AiBotsNameGen::Generate();
            if (!IsNameTaken(candidate))
                return candidate;
        }
    }

    // Режим 0 (или генератор не помог): числовой суффикс.
    std::string base = candidates[urand(0, candidates.size() - 1)];
    for (uint32 suffix = 2; suffix < 100; ++suffix)
    {
        // Ограничение имени персонажа — 12 символов.
        std::string candidate = base.substr(0, 12 - std::to_string(suffix).length()) + std::to_string(suffix);
        if (!IsNameTaken(candidate))
            return candidate;
    }

    // Крайне маловероятно: используем guid как суффикс.
    return base.substr(0, 6) + std::to_string(ObjectMgr::GenerateLowGuid(HIGHGUID_PLAYER));
}

bool BotMgr::IsNameTaken(std::string const& name) const
{
    // 1. Имя уже у бота в памяти (создан в текущей сессии).
    for (auto const& pair : _allBots)
        if (pair.second.name == name)
            return true;

    // 2. Имя занято другим персонажем в БД.
    if (sCharacterCache->GetCharacterCacheByName(name))
        return true;

    return false;
}

void BotMgr::LoadBotNames()
{
    _botNames.clear();

    QueryResult result = WorldDatabase.Query("SELECT `name` FROM `mod_ai_bots_names`");
    if (!result)
    {
        LOG_WARN("server.loading", ">> mod-ai-bots: таблица mod_ai_bots_names пуста или отсутствует — используются встроенные имена.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        std::string name = fields[0].Get<std::string>();
        if (!name.empty())
            _botNames.push_back(name);
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> mod-ai-bots: загружено {} имён ботов из mod_ai_bots_names.", _botNames.size());
}

bool BotMgr::CreateBotForPlayer(Player* owner, uint8 race, uint8 cls)
{
    if (!owner || !AiBotsConfig::IsEnabled())
        return false;

    ObjectGuid::LowType ownerGuid = owner->GetGUID().GetCounter();

    if (GetBotCountForPlayer(ownerGuid) >= AiBotsConfig::GetMaxBotsPerPlayer())
    {
        ChatHandler(owner->GetSession()).PSendSysMessage("[AI-Bot] Достигнут лимит ботов ({}).", AiBotsConfig::GetMaxBotsPerPlayer());
        return false;
    }

    // Если раса/класс не заданы — берём из конфига или случайные.
    if (!race || !cls)
    {
        uint8 cfgRace, cfgCls;
        ParseRaceClass(AiBotsConfig::GetDefaultRaceClass(), cfgRace, cfgCls);
        race = cfgRace;
        cls = cfgCls;
    }

    if (!IsValidRaceClass(race, cls))
    {
        ChatHandler(owner->GetSession()).PSendSysMessage("[AI-Bot] Недопустимая комбинация раса/класс ({}:{})", race, cls);
        return false;
    }

    std::string name = GenerateUniqueBotName();

    // Уровень бота: синхронизация с владельцем либо фиксированный.
    uint8 level = 1;
    if (AiBotsConfig::GetSyncOwnerLevel() > 0 && owner)
        level = static_cast<uint8>(std::min<uint32>(owner->GetLevel(), AiBotsConfig::GetSyncOwnerLevel()));

    // Случайная внешность: у каждого бота свой облик.
    uint8 skin, face, hairStyle, hairColor, facialFeature, tattoos;
    AiBotsGear::RandomizeAppearance(skin, face, hairStyle, hairColor, facialFeature, tattoos);

    ObjectGuid::LowType lowGuid = ObjectMgr::GenerateLowGuid(HIGHGUID_PLAYER);

    // Создаём запись персонажа в БД (упрощённо, минимальный набор полей).
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER);
    stmt->setUInt32(0, lowGuid);
    stmt->setUInt32(1, BotAccount);
    stmt->setString(2, name);
    stmt->setUInt8(3, race);
    stmt->setUInt8(4, cls);
    stmt->setUInt8(5, owner->getGender());
    stmt->setUInt8(6, level);
    stmt->setUInt8(7, skin);
    stmt->setUInt8(8, face);
    stmt->setUInt8(9, hairStyle);
    stmt->setUInt8(10, hairColor);
    stmt->setUInt8(11, facialFeature);
    stmt->setUInt8(12, tattoos);

    CharacterDatabase.AsyncExecute(stmt);

    BotData data;
    data.guid = lowGuid;
    data.ownerGuid = ownerGuid;
    data.name = name;
    data.race = race;
    data.cls = cls;
    data.level = level;
    data.inWorld = false;

    _botsByOwner[ownerGuid].push_back(data);
    _allBots[lowGuid] = data;

    ChatHandler(owner->GetSession()).PSendSysMessage("[AI-Bot] Бот '{}' создан (раса {}, класс {}). Ожидание входа в мир...", name, race, cls);

    // Откладываем вход в мир, чтобы запись успела записаться в БД.
    ScheduleEnterWorld(lowGuid, ownerGuid);

    return true;
}

void BotMgr::ScheduleEnterWorld(ObjectGuid::LowType botGuid, ObjectGuid::LowType ownerGuid)
{
    _pendingLogins.push_back({ botGuid, ownerGuid, 3000 }); // 3 секунды на запись в БД
}

void BotMgr::HandleBotLogin(ObjectGuid::LowType botGuid)
{
    auto it = _allBots.find(botGuid);
    if (it == _allBots.end())
        return;

    BotData& data = it->second;
    data.inWorld = true;

    Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botGuid));
    if (bot)
    {
        // Выдаём экипировку по классу (если слоты пусты).
        AiBotsGear::EquipGearFor(bot);

        Player* owner = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(data.ownerGuid));
        if (owner)
            ChatHandler(owner->GetSession()).PSendSysMessage("[AI-Bot] Бот '{}' вошёл в мир.", data.name);
    }
}

bool BotMgr::EnterWorld(ObjectGuid::LowType botGuid, ObjectGuid::LowType ownerGuid)
{
    // Полноценная загрузка персонажа из БД асинхронным LoginQueryHolder
    // выполняется в LoginPlayerHolder-пайплайне ядра; здесь мы просто
    // инициируем вход через WorldSession-механизм ядра.
    auto it = _allBots.find(botGuid);
    if (it == _allBots.end())
        return false;

    Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botGuid));
    if (bot)
        return true; // уже в мире

    // Запрос на загрузку персонажа из БД.
    LoginQueryHolder* holder = new LoginQueryHolder(BotAccount, ObjectGuid::Create<HighGuid::Player>(botGuid));
    if (!holder->Initialize())
    {
        delete holder;
        return false;
    }

    CharacterDatabase.DelayQueryHolder(holder);
    return true;
}

bool BotMgr::RemoveBot(Player* owner, std::string const& botName)
{
    if (!owner)
        return false;

    ObjectGuid::LowType ownerGuid = owner->GetGUID().GetCounter();
    auto& bots = _botsByOwner[ownerGuid];

    for (auto it = bots.begin(); it != bots.end(); ++it)
    {
        if (it->name != botName)
            continue;

        ObjectGuid::LowType botGuid = it->guid;

        if (Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botGuid)))
            bot->RemoveFromWorld();

        CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER);
        stmt->setUInt32(0, botGuid);
        CharacterDatabase.AsyncExecute(stmt);

        _allBots.erase(botGuid);
        bots.erase(it);

        ChatHandler(owner->GetSession()).PSendSysMessage("[AI-Bot] Бот '{}' удалён.", botName);
        return true;
    }

    ChatHandler(owner->GetSession()).PSendSysMessage("[AI-Bot] Бот '{}' не найден.", botName);
    return false;
}

void BotMgr::RemoveAllBots(Player* owner)
{
    if (!owner)
        return;

    ObjectGuid::LowType ownerGuid = owner->GetGUID().GetCounter();
    auto bots = _botsByOwner[ownerGuid]; // копия, т.к. RemoveBot меняет контейнер
    for (auto const& data : bots)
        RemoveBot(owner, data.name);
}

void BotMgr::LogoutBot(ObjectGuid::LowType botGuid)
{
    if (Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botGuid)))
        bot->GetSession()->LogoutPlayer(true);

    auto it = _allBots.find(botGuid);
    if (it != _allBots.end())
        it->second.inWorld = false;
}

void BotMgr::Update(uint32 diff)
{
    if (!AiBotsConfig::IsEnabled())
        return;

    _updateTimer += diff;
    if (_updateTimer < AiBotsConfig::GetUpdateIntervalMs())
        return;
    _updateTimer = 0;

    // Отложенные входы в мир
    for (auto it = _pendingLogins.begin(); it != _pendingLogins.end();)
    {
        PendingLogin& p = *it;
        p.remainingMs -= AiBotsConfig::GetUpdateIntervalMs();

        if (p.remainingMs > AiBotsConfig::GetUpdateIntervalMs())
        {
            ++it;
            continue;
        }

        if (EnterWorld(p.botGuid, p.ownerGuid))
            HandleBotLogin(p.botGuid);

        it = _pendingLogins.erase(it);
    }

    // Если владелец вышел из мира — боты тоже выходят.
    for (auto it = _botsByOwner.begin(); it != _botsByOwner.end();)
    {
        ObjectGuid::LowType ownerGuid = it->first;
        bool ownerOnline = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(ownerGuid)) != nullptr;

        if (!ownerOnline)
        {
            for (auto const& data : it->second)
                if (data.inWorld)
                    LogoutBot(data.guid);
            it = _botsByOwner.erase(it);
        }
        else
            ++it;
    }
}

Player* BotMgr::GetBotPlayer(ObjectGuid::LowType botGuid) const
{
    auto it = _allBots.find(botGuid);
    if (it == _allBots.end() || !it->second.inWorld)
        return nullptr;
    return ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botGuid));
}

std::vector<BotData> const& BotMgr::GetBotsOfOwner(ObjectGuid::LowType ownerGuid) const
{
    static std::vector<BotData> const empty;
    auto it = _botsByOwner.find(ownerGuid);
    return it != _botsByOwner.end() ? it->second : empty;
}

std::string const& BotMgr::GetBotName(ObjectGuid::LowType botGuid) const
{
    static std::string const empty;
    auto it = _allBots.find(botGuid);
    return it != _allBots.end() ? it->second.name : empty;
}

void BotMgr::HandlePlayerLogout(Player* player)
{
    if (!player)
        return;

    // Выходим боты владельца (данные сохраняются, боты остаются в БД).
    ObjectGuid::LowType ownerGuid = player->GetGUID().GetCounter();
    auto it = _botsByOwner.find(ownerGuid);
    if (it == _botsByOwner.end())
        return;

    for (auto const& data : it->second)
        if (data.inWorld)
            LogoutBot(data.guid);
}

void BotMgr::HandlePlayerLogin(Player* player)
{
    if (!player || !AiBotsConfig::IsEnabled() || !AiBotsConfig::IsAutoSpawnEnabled())
        return;

    uint32 count = AiBotsConfig::GetBotsPerLogin();
    for (uint32 i = 0; i < count; ++i)
        CreateBotForPlayer(player);
}

uint32 BotMgr::GetBotCountForPlayer(ObjectGuid::LowType ownerGuid) const
{
    auto it = _botsByOwner.find(ownerGuid);
    return it != _botsByOwner.end() ? static_cast<uint32>(it->second.size()) : 0;
}
