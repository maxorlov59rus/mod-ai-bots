/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotSiege.cpp
 *
 * Цели осад загружаются из таблицы mod_ai_bots_siege_targets
 * (настраиваются без пересборки); при пустой таблице — встроенный список.
 *
 * Цикл осады:
 *   1. Раз в Interval (90–240 мин) запускается осада случайного города;
 *   2. Отбирается отряд AttackerMin..AttackerMax автономных ботов
 *      противоположной фракции, они телепортируются к воротам города;
 *   3. Анонс в мир + боевые кличи;
 *   4. Боты сражаются с защитниками (штатный FindTarget/FindPvP);
 *   5. Через DurationMinutes — отступление: телепорт домой, анонс.
 */

#include "BotSiege.h"
#include "BotMgr.h"
#include "Config_Bots.h"

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "MapMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "World.h"

#include <vector>

namespace
{
    // Fallback: встроенные цели, если таблица пуста/отсутствует.
    std::vector<BotSiegeTarget> const DefaultTargets =
    {
        { 0, -9050.0f,   440.0f, 94.0f, "Stormwind",   TEAM_ALLIANCE },
        { 0, -4981.0f,  -881.0f, 501.0f, "Ironforge",   TEAM_ALLIANCE },
        { 1,  1568.0f, -4297.0f,  61.0f, "Orgrimmar",   TEAM_HORDE },
        { 1,  1831.0f,   238.0f,  61.0f, "Undercity",   TEAM_HORDE },
    };

    std::vector<BotSiegeTarget> s_targets;
    bool s_targetsLoaded = false;

    std::vector<BotSiegeTarget> const& GetTargetsInternal()
    {
        if (!s_targetsLoaded)
        {
            s_targets.clear();

            if (QueryResult result = WorldDatabase.Query(
                "SELECT `mapId`, `x`, `y`, `z`, `name`, `faction` FROM `mod_ai_bots_siege_targets`"))
            {
                do
                {
                    Field* fields = result->Fetch();
                    s_targets.push_back(
                    {
                        fields[0].Get<uint32>(),
                        fields[1].Get<float>(),
                        fields[2].Get<float>(),
                        fields[3].Get<float>(),
                        fields[4].Get<std::string>(),
                        fields[5].Get<uint32>() == 1 ? TEAM_HORDE : TEAM_ALLIANCE
                    });
                } while (result->NextRow());

                LOG_INFO("server.loading", ">> mod-ai-bots: загружено {} целей осад из mod_ai_bots_siege_targets.", s_targets.size());
            }

            if (s_targets.empty())
            {
                LOG_WARN("server.loading", ">> mod-ai-bots: таблица mod_ai_bots_siege_targets пуста — используются встроенные цели.");
                s_targets = DefaultTargets;
            }

            s_targetsLoaded = true;
        }

        return s_targets;
    }

    enum SiegeState
    {
        SIEGE_IDLE = 0,
        SIEGE_RUNNING
    };

    SiegeState s_state = SIEGE_IDLE;
    uint32 s_nextSiegeMs = 0;      // до следующей осады
    uint32 s_siegeTimerMs = 0;     // длительность текущей осады
    std::vector<ObjectGuid::LowType> s_attackers;

    void Announce(std::string const& text)
    {
        sWorld->SendWorldText(LANG_SYSTEMMESSAGE, text.c_str());
    }

    void ReturnHome(Player* bot)
    {
        bot->RemoveFromWorld();
        bot->SaveToDB(false, false);
    }
}

namespace AiBotsSiege
{
    std::vector<BotSiegeTarget> const& GetTargets()
    {
        return GetTargetsInternal();
    }

    bool IsSiegeActive()
    {
        return s_state == SIEGE_RUNNING;
    }

    void Init()
    {
        if (!AiBotsConfig::IsSiegeEnabled())
            return;

        // Первая осада — через случайный интервал.
        s_nextSiegeMs = urand(AiBotsConfig::GetSiegeIntervalMin(), AiBotsConfig::GetSiegeIntervalMax()) * 60 * 1000;
        LOG_INFO("server.loading", ">> mod-ai-bots: первая осада города через {} минут.",
            s_nextSiegeMs / 60000);
    }

    void StartSiege()
    {
        if (s_state != SIEGE_IDLE)
            return;

        auto const& allBots = sBotMgr->GetAllBots();

        // Свободные (не в бою) живые боты.
        std::vector<Player*> available;
        for (auto const& pair : allBots)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(pair.first)))
                if (bot->IsInWorld() && bot->IsAlive() && !bot->IsInCombat())
                    available.push_back(bot);
        }

        if (available.empty())
            return;

        // Случайная цель и фракция атакующих.
        BotSiegeTarget const& target = GetTargetsInternal()[urand(0, GetTargetsInternal().size() - 1)];
        TeamId attackingTeam = (target.faction == TEAM_HORDE) ? TEAM_ALLIANCE : TEAM_HORDE;

        // Отбираем отряд нужной фракции.
        std::vector<Player*> squad;
        uint32 wanted = urand(AiBotsConfig::GetSiegeAttackerMin(), AiBotsConfig::GetSiegeAttackerMax());
        for (Player* bot : available)
        {
            if (squad.size() >= wanted)
                break;
            if (bot->GetTeamId() != attackingTeam)
                continue;
            squad.push_back(bot);
        }

        if (squad.size() < 2)
            return; // не набрали отряд

        // Телепорт к воротам города.
        s_attackers.clear();
        for (Player* bot : squad)
        {
            float angle = frand(0.0f, 2.0f * float(M_PI));
            bot->NearTeleportTo(
                target.x + 5.0f * std::cos(angle),
                target.y + 5.0f * std::sin(angle),
                target.z, frand(0.0f, 2.0f * float(M_PI)));
            s_attackers.push_back(bot->GetGUID().GetCounter());
        }

        s_state = SIEGE_RUNNING;
        s_siegeTimerMs = AiBotsConfig::GetSiegeDurationMinutes() * 60 * 1000;

        std::string attackerName = (attackingTeam == TEAM_HORDE) ? "Horde" : "Alliance";
        Announce("[AI-Bots] The " + attackerName + " forces are storming " + target.name + "!");
        LOG_INFO("server.loading", ">> mod-ai-bots: осада '{}' начата, атакующих: {}.",
            target.name, s_attackers.size());
    }

    void EndSiege(bool retreat)
    {
        s_state = SIEGE_IDLE;
        s_attackers.clear();

        // Следующая осада через случайный интервал.
        s_nextSiegeMs = urand(AiBotsConfig::GetSiegeIntervalMin(), AiBotsConfig::GetSiegeIntervalMax()) * 60 * 1000;

        if (retreat)
            Announce("[AI-Bots] The siege forces have withdrawn. The city stands!");
    }

    void Update(uint32 diff)
    {
        if (!AiBotsConfig::IsSiegeEnabled())
            return;

        switch (s_state)
        {
            case SIEGE_IDLE:
            {
                uint32 intervalMs = AiBotsConfig::GetUpdateIntervalMs();
                if (s_nextSiegeMs > intervalMs)
                {
                    s_nextSiegeMs -= intervalMs;
                    return;
                }
                s_nextSiegeMs = 0;
                StartSiege();
                break;
            }

            case SIEGE_RUNNING:
            {
                // Отсчёт длительности.
                uint32 intervalMs = AiBotsConfig::GetUpdateIntervalMs();
                if (s_siegeTimerMs > intervalMs)
                    s_siegeTimerMs -= intervalMs;
                else
                {
                    // Время вышло — отступление.
                    EndSiege(true);
                    break;
                }

                // Если атакующих в живых не осталось — осада провалена.
                uint32 aliveCount = 0;
                for (auto guid : s_attackers)
                {
                    if (Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(guid)))
                        if (bot->IsInWorld() && bot->IsAlive())
                            ++aliveCount;
                }

                if (aliveCount == 0)
                    EndSiege(true);

                break;
            }
        }
    }
}
