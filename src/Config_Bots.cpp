/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/Config_Bots.cpp
 */

#include "Config_Bots.h"

#include "Config.h"

namespace
{
    bool        s_enabled       = false;
    uint32      s_maxBots       = 1;
    bool        s_autoSpawn     = false;
    uint32      s_botsPerLogin  = 1;
    uint32      s_updateMs      = 500;
    bool        s_combat        = true;
    float       s_followDist    = 4.0f;
    bool        s_cmdAll        = true;
    std::string s_defaultRC     = "1:1"; // human warrior

    // Автономные боты
    bool        s_worldEnabled      = false;
    uint32      s_worldCount        = 10;
    uint8       s_worldSpawnMode    = 0;
    uint32      s_worldRespawnSec   = 60;
    bool        s_worldSpells       = true;
    bool        s_worldChat         = true;
    uint32      s_battleCryChance   = 30;
    uint32      s_idleChatChance    = 2;

    // Прокачка и тактика
    uint32      s_worldBotLevel     = 5;
    uint32      s_syncOwnerLevel    = 0;
    uint32      s_fleeHealthPct     = 20;
    uint32      s_healThresholdPct  = 50;
    bool        s_useBuffs          = true;

    // Генерация имён
    uint8       s_namesMode         = 2;
}

namespace AiBotsConfig
{
    void Load()
    {
        s_enabled      = sConfigMgr->GetOption<bool>("AiBots.Enable", false);
        s_maxBots      = sConfigMgr->GetOption<uint32>("AiBots.MaxBotsPerPlayer", 1);
        s_autoSpawn    = sConfigMgr->GetOption<bool>("AiBots.AutoSpawn", false);
        s_botsPerLogin = sConfigMgr->GetOption<uint32>("AiBots.AutoSpawnCount", 1);
        s_updateMs     = sConfigMgr->GetOption<uint32>("AiBots.UpdateInterval", 500);
        s_combat       = sConfigMgr->GetOption<bool>("AiBots.EnableCombat", true);
        s_followDist   = sConfigMgr->GetOption<float>("AiBots.FollowDistance", 4.0f);
        s_cmdAll       = sConfigMgr->GetOption<bool>("AiBots.CommandsForAll", true);
        s_defaultRC    = sConfigMgr->GetOption<std::string>("AiBots.DefaultRaceClass", "1:1");

        s_worldEnabled     = sConfigMgr->GetOption<bool>("AiBots.WorldBots.Enable", false);
        s_worldCount       = sConfigMgr->GetOption<uint32>("AiBots.WorldBots.Count", 10);
        s_worldSpawnMode   = sConfigMgr->GetOption<uint8>("AiBots.WorldBots.SpawnMode", 0);
        s_worldRespawnSec  = sConfigMgr->GetOption<uint32>("AiBots.WorldBots.RespawnSeconds", 60);
        s_worldSpells      = sConfigMgr->GetOption<bool>("AiBots.WorldBots.UseSpells", true);
        s_worldChat        = sConfigMgr->GetOption<bool>("AiBots.WorldBots.Chat.Enable", true);
        s_battleCryChance  = sConfigMgr->GetOption<uint32>("AiBots.WorldBots.Chat.BattleCryChance", 30);
        s_idleChatChance   = sConfigMgr->GetOption<uint32>("AiBots.WorldBots.Chat.IdleChance", 2);

        s_worldBotLevel    = sConfigMgr->GetOption<uint32>("AiBots.WorldBots.Level", 5);
        s_syncOwnerLevel   = sConfigMgr->GetOption<uint32>("AiBots.SyncOwnerLevel", 0);
        s_fleeHealthPct    = sConfigMgr->GetOption<uint32>("AiBots.Tactics.FleeHealthPct", 20);
        s_healThresholdPct = sConfigMgr->GetOption<uint32>("AiBots.Tactics.HealThresholdPct", 50);
        s_useBuffs         = sConfigMgr->GetOption<bool>("AiBots.Tactics.UseBuffs", true);

        s_namesMode        = sConfigMgr->GetOption<uint8>("AiBots.Names.Mode", 2);
    }

    bool IsEnabled()                    { return s_enabled; }
    uint32 GetMaxBotsPerPlayer()        { return s_maxBots; }
    bool IsAutoSpawnEnabled()           { return s_autoSpawn; }
    uint32 GetBotsPerLogin()            { return s_botsPerLogin; }
    uint32 GetUpdateIntervalMs()        { return s_updateMs; }
    bool IsCombatEnabled()              { return s_combat; }
    float GetFollowDistance()           { return s_followDist; }
    bool IsCommandAvailableToAll()      { return s_cmdAll; }
    std::string const& GetDefaultRaceClass() { return s_defaultRC; }

    bool    IsWorldBotsEnabled()          { return s_worldEnabled; }
    uint32  GetWorldBotsCount()           { return s_worldCount; }
    uint8   GetWorldBotsSpawnMode()       { return s_worldSpawnMode; }
    uint32  GetWorldBotsRespawnSeconds()  { return s_worldRespawnSec; }
    bool    IsWorldBotsSpellsEnabled()    { return s_worldSpells; }
    bool    IsWorldBotsChatEnabled()      { return s_worldChat; }
    uint32  GetBattleCryChance()          { return s_battleCryChance; }
    uint32  GetIdleChatChance()           { return s_idleChatChance; }

    uint32  GetWorldBotLevel()            { return s_worldBotLevel; }
    uint32  GetSyncOwnerLevel()           { return s_syncOwnerLevel; }
    uint32  GetFleeHealthPct()            { return s_fleeHealthPct; }
    uint32  GetHealThresholdPct()         { return s_healThresholdPct; }
    bool    IsBuffsEnabled()              { return s_useBuffs; }

    uint8   GetNamesMode()                { return s_namesMode; }
}
