/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/Config_Bots.h
 *
 * Интерфейс конфигурации модуля.
 */

#ifndef AI_BOTS_CONFIG_BOTS_H
#define AI_BOTS_CONFIG_BOTS_H

#include "Define.h"

#include <cstdint>
#include <string>

namespace AiBotsConfig
{
    // Загрузка/перечитывание конфигурации
    void Load();

    bool    IsEnabled();            // AiBots.Enable
    uint32  GetMaxBotsPerPlayer();  // AiBots.MaxBotsPerPlayer
    bool    IsAutoSpawnEnabled();   // AiBots.AutoSpawn
    uint32  GetBotsPerLogin();      // AiBots.AutoSpawnCount
    uint32  GetUpdateIntervalMs();  // AiBots.UpdateInterval
    bool    IsCombatEnabled();      // AiBots.EnableCombat
    float   GetFollowDistance();    // AiBots.FollowDistance
    bool    IsCommandAvailableToAll(); // AiBots.CommandsForAll
    std::string const& GetDefaultRaceClass(); // AiBots.DefaultRaceClass, формат "race:class"

    // Автономные боты в мире (AiBots.WorldBots.*)
    bool    IsWorldBotsEnabled();       // AiBots.WorldBots.Enable
    uint32  GetWorldBotsCount();        // AiBots.WorldBots.Count
    uint8   GetWorldBotsSpawnMode();    // AiBots.WorldBots.SpawnMode: 0=SQL, 1=у игроков, 2=стартовые зоны
    uint32  GetWorldBotsRespawnSeconds(); // AiBots.WorldBots.RespawnSeconds
    bool    IsWorldBotsSpellsEnabled(); // AiBots.WorldBots.UseSpells
    bool    IsWorldBotsChatEnabled();   // AiBots.WorldBots.Chat.Enable
    uint32  GetBattleCryChance();       // AiBots.WorldBots.Chat.BattleCryChance
    uint32  GetIdleChatChance();        // AiBots.WorldBots.Chat.IdleChance

    // Прокачка и тактика (AiBots.Tactics.*)
    uint32  GetWorldBotLevel();         // AiBots.WorldBots.Level
    uint32  GetSyncOwnerLevel();        // AiBots.SyncOwnerLevel
    uint32  GetFleeHealthPct();         // AiBots.Tactics.FleeHealthPct
    uint32  GetHealThresholdPct();      // AiBots.Tactics.HealThresholdPct
    bool    IsBuffsEnabled();           // AiBots.Tactics.UseBuffs

    // Генерация имён
    uint8   GetNamesMode();             // AiBots.Names.Mode: 0=таблица, 1=генератор, 2=смешанный
}

#endif // AI_BOTS_CONFIG_BOTS_H
