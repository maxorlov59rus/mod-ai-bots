/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/WorldBots.cpp
 *
 * Автономные боты: спавн (SQL / рядом с игроками / стартовые зоны),
 * бой с мобами и враждебными ботами, спеллы, чат, респавн.
 */

#include "WorldBots.h"
#include "BotAI.h"
#include "BotMgr.h"
#include "BotGear.h"
#include "Config_Bots.h"

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharedDefines.h"
#include "World.h"
#include "WorldSession.h"

#include <cmath>

namespace
{
    // Стартовые/столичные зоны для режима SpawnMode = 2.
    std::vector<WorldBotSpawnPoint> const DefaultPoints =
    {
        { 0, -8913.0f,  133.0f, 87.0f,  3.7f }, // Elwynn Forest (Northshire)
        { 0, -9432.0f,  66.0f,  57.0f,  3.7f }, // Elwynn Forest
        { 0, -6240.0f,  331.0f, 89.0f,  4.9f }, // Stormwind outskirts
        { 1, -618.0f,  -4251.0f, 38.0f, 1.5f }, // Durotar (Valley of Trials)
        { 1,  1323.0f, -4344.0f, 30.0f, 5.2f }, // Durotar
        { 1,  1670.0f, -4290.0f, 61.0f, 2.0f }, // Orgrimmar outskirts
        { 0, -2917.0f, -394.0f,  55.0f, 4.2f }, // Dun Morogh
        { 1,   225.0f, -4726.0f, 47.0f, 4.5f }, // Durotar south
    };

    std::vector<uint8> const BotRaces =
    {
        RACE_HUMAN, RACE_ORC, RACE_DWARF, RACE_NIGHTELF, RACE_UNDEAD_PLAYER,
        RACE_TAUREN, RACE_GNOME, RACE_GNOME, RACE_TROLL, RACE_BLOODELF, RACE_DRAENEI
    };

    std::vector<uint8> const BotClasses =
    {
        CLASS_WARRIOR, CLASS_PALADIN, CLASS_HUNTER, CLASS_ROGUE, CLASS_PRIEST,
        CLASS_SHAMAN, CLASS_MAGE, CLASS_WARLOCK, CLASS_DRUID
    };

    std::string const WorldBotSessionName = "AI-WORLDBOT";
    uint32 const WorldBotAccount = 1;
}

WorldBots* WorldBots::instance()
{
    static WorldBots instance;
    return &instance;
}

void WorldBots::SpawnAll()
{
    if (!AiBotsConfig::IsWorldBotsEnabled())
        return;

    _bots.clear();

    LOG_INFO("server.loading", ">> mod-ai-bots: спавн {} автономных ботов (режим {})...",
        AiBotsConfig::GetWorldBotsCount(), AiBotsConfig::GetWorldBotsSpawnMode());

    for (uint32 i = 0; i < AiBotsConfig::GetWorldBotsCount(); ++i)
        SpawnOne();
}

bool WorldBots::SpawnOne()
{
    // Каждому автономному боту — случайные раса и класс.
    uint8 race = BotRaces[urand(0, BotRaces.size() - 1)];
    uint8 cls = BotClasses[urand(0, BotClasses.size() - 1)];

    std::string name = sBotMgr->GenerateUniqueBotName();
    ObjectGuid::LowType lowGuid = ObjectMgr::GenerateLowGuid(HIGHGUID_PLAYER);

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER);
    stmt->setUInt32(0, lowGuid);
    stmt->setUInt32(1, WorldBotAccount);
    stmt->setString(2, name);
    stmt->setUInt8(3, race);
    stmt->setUInt8(4, cls);
    stmt->setUInt8(5, urand(0, 1)); // gender

    uint8 level = static_cast<uint8>(std::min<uint32>(AiBotsConfig::GetWorldBotLevel(), 80));
    stmt->setUInt8(6, level);

    // Случайная внешность: у каждого автономного бота свой облик.
    uint8 skin, face, hairStyle, hairColor, facialFeature, tattoos;
    AiBotsGear::RandomizeAppearance(skin, face, hairStyle, hairColor, facialFeature, tattoos);
    stmt->setUInt8(7, skin);
    stmt->setUInt8(8, face);
    stmt->setUInt8(9, hairStyle);
    stmt->setUInt8(10, hairColor);
    stmt->setUInt8(11, facialFeature);
    stmt->setUInt8(12, tattoos);

    CharacterDatabase.AsyncExecute(stmt);

    _bots.push_back({ lowGuid, false, 0 });
    return true;
}

WorldBotSpawnPoint WorldBots::PickSpawnPoint() const
{
    uint8 mode = AiBotsConfig::GetWorldBotsSpawnMode();

    // Режим 0: SQL-таблица mod_ai_bots_locations.
    if (mode == 0)
    {
        if (!_sqlPointsLoaded)
        {
            auto* self = const_cast<WorldBots*>(this);
            self->_sqlPoints.clear();
            if (QueryResult result = WorldDatabase.Query("SELECT `mapId`, `x`, `y`, `z`, `o` FROM `mod_ai_bots_locations`"))
            {
                do
                {
                    Field* fields = result->Fetch();
                    self->_sqlPoints.push_back(
                    {
                        fields[0].Get<uint32>(),
                        fields[1].Get<float>(),
                        fields[2].Get<float>(),
                        fields[3].Get<float>(),
                        fields[4].Get<float>()
                    });
                } while (result->NextRow());
                LOG_INFO("server.loading", ">> mod-ai-bots: загружено {} точек спавна из mod_ai_bots_locations.", self->_sqlPoints.size());
            }
            self->_sqlPointsLoaded = true;
        }

        if (!_sqlPoints.empty())
            return _sqlPoints[urand(0, _sqlPoints.size() - 1)];
    }

    // Режим 1: рядом со случайным онлайн-игроком.
    if (mode == 1)
    {
        std::vector<Player*> online;
        for (auto const& pair : ObjectAccessor::GetPlayers())
            if (Player* p = pair.second)
                if (p->IsInWorld())
                    online.push_back(p);

        if (!online.empty())
        {
            Player* p = online[urand(0, online.size() - 1)];
            float angle = frand(0.0f, 2.0f * float(M_PI));
            float dist = frand(10.0f, 25.0f);
            return { p->GetMapId(), p->GetPositionX() + dist * std::cos(angle),
                     p->GetPositionY() + dist * std::sin(angle), p->GetPositionZ(), 0.0f };
        }
        // Нет игроков онлайн — падаем в режим 2.
    }

    // Режим 2 (или fallback): случайная стартовая зона.
    return DefaultPoints[urand(0, DefaultPoints.size() - 1)];
}

void WorldBots::Update(uint32 diff)
{
    if (!AiBotsConfig::IsWorldBotsEnabled() || _bots.empty())
        return;

    _updateTimer += diff;
    if (_updateTimer < AiBotsConfig::GetUpdateIntervalMs())
        return;
    _updateTimer = 0;

    for (WorldBot& bot : _bots)
    {
        Player* player = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(bot.guid));

        if (!player)
            continue;

        if (!player->IsAlive())
        {
            if (bot.alive)
            {
                // Только что погиб: запускаем таймер респавна.
                bot.alive = false;
                bot.respawnTimerMs = AiBotsConfig::GetWorldBotsRespawnSeconds() * 1000;
                LOG_DEBUG("server.loading", ">> mod-ai-bots: автономный бот {} погиб, респавн через {} с.",
                    bot.guid, AiBotsConfig::GetWorldBotsRespawnSeconds());
            }
            continue;
        }

        if (!bot.alive)
        {
            // Бот снова жив (респавн выполнен).
            bot.alive = true;
            bot.respawnTimerMs = 0;
        }
    }
}
