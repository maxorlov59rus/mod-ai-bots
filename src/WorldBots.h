/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/WorldBots.h
 *
 * Менеджер автономных ботов: спавн в локациях по режиму из конфига,
 * респавн после смерти, обновление AI.
 */

#ifndef AI_BOTS_WORLD_BOTS_H
#define AI_BOTS_WORLD_BOTS_H

#include "Define.h"

#include <cstdint>
#include <vector>

class Player;

struct WorldBotSpawnPoint
{
    uint32 mapId;
    float x;
    float y;
    float z;
    float o;
};

class WorldBots
{
public:
    static WorldBots* instance();

    // Спавн всех автономных ботов при старте мира.
    void SpawnAll();

    // Периодическое обновление: респавн погибших, тик AI.
    void Update(uint32 diff);

    // Точка спавна согласно режиму из конфига.
    WorldBotSpawnPoint PickSpawnPoint() const;

private:
    WorldBots() = default;

    bool SpawnOne();
    void DespawnAll();

    struct WorldBot
    {
        ObjectGuid::LowType guid;
        bool alive;
        uint32 respawnTimerMs;
    };

    std::vector<WorldBot> _bots;
    std::vector<WorldBotSpawnPoint> _sqlPoints;
    bool _sqlPointsLoaded = false;
    uint32 _updateTimer = 0;
};

#define sWorldBots WorldBots::instance()

#endif // AI_BOTS_WORLD_BOTS_H
