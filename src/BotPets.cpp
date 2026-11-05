/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotPets.cpp
 *
 * Призыв через штатные спеллы ядра: у охотника — Call Pet (id 883,
 * питомец из stable; при отсутствии используется Beast Mastery-суммон),
 * у чернокнижника — Fel Domination-набор демонов (Summon Imp 688,
 * Summon Voidwalker 697, Summon Succubus 712, Summon Felhunter 691).
 * Питомец — настоящий Pet ядра: следует за ботом, сражается с целью.
 */

#include "BotPets.h"
#include "BotMgr.h"
#include "Config_Bots.h"

#include "Log.h"
#include "ObjectAccessor.h"
#include "Pet.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Unit.h"

#include <map>

namespace
{
    // guid бота -> таймер до следующей попытки суммона.
    std::map<ObjectGuid::LowType, uint32> s_summonTimers;

    // Спеллы суммона демонов чернокнижника (ранги подходящего уровня).
    std::vector<uint32> const WarlockPetSpells = { 688, 697, 712, 691 }; // Imp, Voidwalker, Succubus, Felhunter

    uint32 const SummonRetryMs = 2000; // проверка раз в N мс внутри интервала

    bool TryCastSummon(Player* bot, uint32 spellId)
    {
        if (!bot->HasSpell(spellId))
            return false;

        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (!info)
            return false;

        if (!bot->IsSpellReady(spellId) || bot->HasSpellCooldown(spellId))
            return false;

        bot->CastSpell(bot, spellId, true);
        LOG_DEBUG("server.loading", ">> mod-ai-bots: бот {} призывает питомца (спелл {}).",
            bot->GetName(), spellId);
        return true;
    }
}

namespace AiBotsPets
{
    bool ClassHasPet(uint8 cls)
    {
        return cls == CLASS_HUNTER || cls == CLASS_WARLOCK;
    }

    bool SummonPetFor(Player* bot)
    {
        if (!bot || !bot->IsAlive())
            return false;

        // Питомец уже есть.
        if (Pet* pet = bot->GetPet())
            return true;

        switch (bot->getClass())
        {
            case CLASS_HUNTER:
                // Call Pet: призывает последнего активного зверя из stable.
                return TryCastSummon(bot, 883);

            case CLASS_WARLOCK:
                // Случайный демон из доступных.
                {
                    std::vector<uint32> available;
                    for (uint32 spellId : WarlockPetSpells)
                        if (bot->HasSpell(spellId))
                            available.push_back(spellId);

                    if (available.empty())
                        return false;

                    return TryCastSummon(bot, available[urand(0, available.size() - 1)]);
                }

            default:
                return false;
        }
    }

    void Update(uint32 /*diff*/)
    {
        if (!AiBotsConfig::IsPetsEnabled())
            return;

        auto const& allBots = sBotMgr->GetAllBots();
        for (auto const& pair : allBots)
        {
            Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(pair.first));
            if (!bot || !bot->IsInWorld())
                continue;

            if (!ClassHasPet(bot->getClass()))
                continue;

            // Питомец есть — таймер не нужен.
            if (bot->GetPet())
            {
                s_summonTimers.erase(pair.first);
                continue;
            }

            // Мёртвый бот не суммонит.
            if (!bot->IsAlive())
                continue;

            // Интервал между попытками суммона.
            auto it = s_summonTimers.find(pair.first);
            if (it == s_summonTimers.end())
            {
                s_summonTimers[pair.first] = AiBotsConfig::GetPetsSummonInterval() * 1000;
                continue;
            }

            uint32& timer = it->second;
            if (timer > SummonRetryMs)
            {
                timer -= SummonRetryMs;
                continue;
            }

            if (SummonPetFor(bot))
                s_summonTimers.erase(pair.first);
            else
                timer = AiBotsConfig::GetPetsSummonInterval() * 1000; // повтор позже
        }
    }
}
