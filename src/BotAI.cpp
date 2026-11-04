/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotAI.cpp
 *
 * Реализация поведения: следование за владельцем, melee-атака, спеллы
 * по классу, PvP бот-против-бота и чат для автономных ботов.
 */

#include "BotAI.h"
#include "BotMgr.h"
#include "BotSpells.h"
#include "BotPersonality.h"
#include "Config_Bots.h"

#include "Cell.h"
#include "CellImpl.h"
#include "Chat.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "SpellMgr.h"
#include "Unit.h"

#include <cmath>

namespace
{
    float const MaxAttackRange = 5.0f;      // дистанция melee-атаки
    float const MaxSpellRange = 30.0f;      // дистанция каста
    float const SearchTargetRange = 30.0f;  // радиус поиска мобов
    float const SearchPvPRange = 40.0f;     // радиус поиска PvP-целей
    uint32 const AttackCooldownMs = 2000;
    uint32 const ChatCheckIntervalMs = 10000;
}

BotAI::BotAI(Player* bot)
    : _bot(bot), _owner(nullptr), _autonomous(false), _attackTimer(0), _chatTimer(0), _buffTimer(0), _fleeTimer(0)
    , _inCombat(false), _fleeing(false), _teamId(TEAM_NEUTRAL)
{
    if (bot)
        _spells = AiBotsSpells::GetSpellsForClass(bot->getClass());
}

void BotAI::UpdateAI(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld() || _bot->IsBeingTeleported())
        return;

    if (_autonomous)
    {
        UpdateAutonomous(diff);
        return;
    }

    // Тактика общая для обоих режимов: лечение/баффы/отступление.
    UpdateTactics(diff);

    if (_fleeing)
    {
        UpdateFleeing(diff);
        return;
    }

    FollowOwner();
    UpdateCombat(diff);
}

void BotAI::FollowOwner()
{
    if (!_owner || !_owner->IsInWorld())
        return;

    float dist = _bot->GetDistance(_owner);
    if (dist > AiBotsConfig::GetFollowDistance() + 2.0f)
        _bot->GetMotionMaster()->MoveFollow(_owner, AiBotsConfig::GetFollowDistance(), 0.0f);
}

void BotAI::UpdateCombat(uint32 diff)
{
    if (!AiBotsConfig::IsCombatEnabled())
        return;

    _attackTimer += diff;
    if (_attackTimer < AttackCooldownMs)
        return;
    _attackTimer = 0;

    // Цель, выбранная самим ботом
    if (Unit* target = _bot->GetSelectedUnit())
    {
        if (target->IsAlive() && target->IsHostileTo(_bot) &&
            _bot->GetDistance(target) <= MaxAttackRange)
        {
            _bot->AttackerStateUpdate(target);
            return;
        }
    }

    if (Unit* target = FindTarget())
        Attack(target);
}

// ---------- Автономный режим ----------

void BotAI::UpdateAutonomous(uint32 diff)
{
    if (!_bot->IsAlive())
    {
        _inCombat = false;
        return;
    }

    bool combatStarted = false;
    Unit* target = FindTarget();
    if (!target)
        target = FindPvPTarget();

    if (target)
    {
        if (!_inCombat && AiBotsConfig::IsWorldBotsChatEnabled())
        {
            SayChat(AiBotsPersonality::GetForGuid(_bot->GetGUID().GetCounter()).battleCries,
                    roll_chance_i(AiBotsConfig::GetBattleCryChance() / 2));
            combatStarted = true;
        }

        _inCombat = true;
        Attack(target);
    }
    else
    {
        if (_inCombat)
        {
            // Бой окончен — фраза победы.
            _inCombat = false;
            if (AiBotsConfig::IsWorldBotsChatEnabled() && roll_chance_i(AiBotsConfig::GetBattleCryChance()))
                SayChat(AiBotsPersonality::GetForGuid(_bot->GetGUID().GetCounter()).victory);
        }
        StopAttack();
    }

    UpdateChat(diff, _inCombat || combatStarted);
}

Unit* BotAI::FindTarget()
{
    // Ищем ближайшего живого моба, враждебного боту.
    std::list<Unit*> targets;
    Acore::AnyUnitInObjectRangeCheck check(_bot, SearchTargetRange);
    Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(_bot, targets, check);
    Cell::VisitAllObjects(_bot, searcher, SearchTargetRange);

    Unit* best = nullptr;
    float bestDist = SearchTargetRange;
    for (Unit* unit : targets)
    {
        if (!unit->IsAlive() || !unit->IsHostileTo(_bot) || unit->IsPlayer())
            continue;

        // Не трогаем существ с пустым индикатором агрессии (декоративные NPC).
        if (unit->GetReactionTo(_bot) == REP_FRIENDLY)
            continue;

        float dist = _bot->GetDistance(unit);
        if (dist < bestDist)
        {
            bestDist = dist;
            best = unit;
        }
    }

    return best;
}

Unit* BotAI::FindPvPTarget()
{
    // PvP: бот враждебной фракции рядом (только автономные боты, не игроки).
    std::list<Player*> targets;
    Acore::AnyPlayerInObjectRangeCheck check(_bot, SearchPvPRange);
    Acore::PlayerListSearcher<Acore::AnyPlayerInObjectRangeCheck> searcher(_bot, targets, check);
    Cell::VisitAllObjects(_bot, searcher, SearchPvPRange);

    Unit* best = nullptr;
    float bestDist = SearchPvPRange;
    for (Player* player : targets)
    {
        if (!player->IsAlive() || player == _bot)
            continue;

        // Боты-игроки без владельца, враждебные по фракции.
        if (!player->IsPvP())
            continue;

        if (!player->IsHostileTo(_bot))
            continue;

        float dist = _bot->GetDistance(player);
        if (dist < bestDist)
        {
            bestDist = dist;
            best = player;
        }
    }

    return best;
}

bool BotAI::CastRandomSpell(Unit* target)
{
    if (_spells.empty() || !target)
        return false;

    // Пробуем случайные спеллы, пока один не скастуется.
    uint32 attempts = _spells.size();
    while (attempts-- > 0)
    {
        uint32 spellId = _spells[urand(0, _spells.size() - 1)];
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            continue;

        if (!_bot->HasSpell(spellId))
            continue;

        float range = spellInfo->GetMaxRange();
        if (_bot->GetDistance(target) > range)
            continue;

        if (_bot->IsSpellReady(spellId) && !_bot->HasSpellCooldown(spellId))
        {
            _bot->CastSpell(target, spellId, true);
            return true;
        }
    }

    return false;
}

void BotAI::UpdateChat(uint32 diff, bool inCombat)
{
    if (!AiBotsConfig::IsWorldBotsChatEnabled() || inCombat)
        return;

    _chatTimer += diff;
    if (_chatTimer < ChatCheckIntervalMs)
        return;
    _chatTimer = 0;

    if (roll_chance_i(AiBotsConfig::GetIdleChatChance()))
        SayChat(AiBotsPersonality::GetForGuid(_bot->GetGUID().GetCounter()).idle);
}

void BotAI::SayChat(std::vector<std::string> const& phrases, bool yell)
{
    if (phrases.empty())
        return;

    std::string const& phrase = phrases[urand(0, phrases.size() - 1)];
    ChatHandler(_bot->GetSession()).PSendSysMessage("%s", phrase.c_str()); // fallback для отладки в консоль

    // Say / Yell в мир — видно другим игрокам вокруг.
    _bot->Say(phrase, LANG_UNIVERSAL);
    if (yell)
        _bot->Yell(phrase, LANG_UNIVERSAL);
}

// ---------- Тактика: лечение, баффы, отступление ----------

void BotAI::UpdateTactics(uint32 diff)
{
    if (!_bot->IsAlive())
        return;

    // Отступление при низком HP.
    if (AiBotsConfig::GetFleeHealthPct() > 0 && ShouldFlee())
    {
        if (!_fleeing)
            StartFleeing(_bot->GetSelectedUnit() ? _bot->GetSelectedUnit() : _bot->GetVictim());
        return;
    }

    // Лечение при низком HP (себя, затем владельца).
    if (AiBotsConfig::GetHealThresholdPct() > 0)
    {
        if (_bot->GetHealthPct() < AiBotsConfig::GetHealThresholdPct())
        {
            if (TryHeal(_bot))
                return;
        }
        else if (_owner && _owner->IsInWorld() && _owner->IsAlive() &&
                 _owner->GetHealthPct() < AiBotsConfig::GetHealThresholdPct())
        {
            TryHeal(_owner);
        }
    }

    // Баффы — раз в 30 секунд проверяем.
    _buffTimer += diff;
    if (AiBotsConfig::IsBuffsEnabled() && _buffTimer >= 30000)
    {
        _buffTimer = 0;
        TryBuff();
    }
}

bool BotAI::TryHeal(Unit* target)
{
    if (!target || !target->IsAlive())
        return false;

    auto const& heals = AiBotsSpells::GetHealSpellsForClass(_bot->getClass());
    if (heals.empty())
        return false;

    for (uint32 spellId : heals)
    {
        if (!_bot->HasSpell(spellId))
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            continue;

        if (!_bot->IsSpellReady(spellId) || _bot->HasSpellCooldown(spellId))
            continue;

        if (_bot->GetDistance(target) > spellInfo->GetMaxRange())
            continue;

        _bot->CastSpell(target, spellId, true);

        // Изредка бот сообщает о лечении в чат.
        if (_autonomous && AiBotsConfig::IsWorldBotsChatEnabled() && roll_chance_i(20))
            SayChat(AiBotsPersonality::GetForGuid(_bot->GetGUID().GetCounter()).heal);

        return true;
    }

    return false;
}

void BotAI::TryBuff()
{
    auto const& buffs = AiBotsSpells::GetBuffSpellsForClass(_bot->getClass());
    if (buffs.empty())
        return;

    for (uint32 spellId : buffs)
    {
        if (!_bot->HasSpell(spellId))
            continue;

        if (!_bot->IsSpellReady(spellId) || _bot->HasSpellCooldown(spellId))
            continue;

        // Кастуем на себя, если бафф ещё не висит.
        if (!_bot->HasAura(spellId))
            _bot->CastSpell(_bot, spellId, true);

        // Бафф владельца, если он в той же фракции и рядом.
        if (_owner && _owner->IsInWorld() && _owner->IsFriendlyTo(_bot) &&
            _bot->GetDistance(_owner) <= 30.0f && !_owner->HasAura(spellId))
        {
            _bot->CastSpell(_owner, spellId, true);
        }
    }
}

bool BotAI::ShouldFlee() const
{
    if (!_bot->IsInCombat())
        return false;

    return _bot->GetHealthPct() < AiBotsConfig::GetFleeHealthPct();
}

void BotAI::StartFleeing(Unit* from)
{
    _fleeing = true;
    _fleeTimer = 8000; // до 8 секунд отступления

    StopAttack();

    if (from && _autonomous && AiBotsConfig::IsWorldBotsChatEnabled() && roll_chance_i(50))
        SayChat(AiBotsPersonality::GetForGuid(_bot->GetGUID().GetCounter()).flee);

    // Убегаем в противоположном направлении от угрозы.
    float fleeAngle = from ? _bot->GetAngle(from) + float(M_PI) : frand(0.0f, 2.0f * float(M_PI));
    float fleeDist = 30.0f;
    float x = _bot->GetPositionX() + fleeDist * std::cos(fleeAngle);
    float y = _bot->GetPositionY() + fleeDist * std::sin(fleeAngle);
    float z = _bot->GetPositionZ();

    _bot->GetMotionMaster()->MovePoint(0, x, y, z);
}

void BotAI::UpdateFleeing(uint32 diff)
{
    // Выходим из режима отступления по таймеру или когда бой окончен.
    if (_fleeTimer <= diff || !_bot->IsInCombat())
    {
        _fleeing = false;
        _fleeTimer = 0;
        if (!_autonomous && _owner && _owner->IsInWorld())
            _bot->GetMotionMaster()->MoveFollow(_owner, AiBotsConfig::GetFollowDistance(), 0.0f);
    }
    else
        _fleeTimer -= diff;
}

// ---------- Общее ----------

void BotAI::Attack(Unit* target)
{
    if (!target || !_bot->IsAlive())
        return;

    _bot->SetSelection(target->GetGUID());

    float dist = _bot->GetDistance(target);
    if (dist > MaxAttackRange)
        _bot->GetMotionMaster()->MoveChase(target, std::min(MaxAttackRange, MaxSpellRange) - 1.0f);

    // Спеллы по классу, если включены.
    if (AiBotsConfig::IsWorldBotsSpellsEnabled() && dist <= MaxSpellRange)
    {
        if (CastRandomSpell(target))
            return;
    }

    // Иначе обычная автоатака.
    if (dist <= MaxAttackRange)
        _bot->AttackerStateUpdate(target);
}

void BotAI::StopAttack()
{
    _bot->AttackStop();
    _bot->GetMotionMaster()->Clear();
}
