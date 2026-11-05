/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotLoot.cpp
 *
 * Виртуальный кошелёк бота: с убитых мобов падает золото и
 * изредка предметы. Экипировка (оружие/броня) сравнивается по item level
 * с надетой — лучшие вещи бот надевает, остальные конвертирует в золото
 * по цене продажи.
 */

#include "BotLoot.h"
#include "BotGuild.h"
#include "Config_Bots.h"

#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Unit.h"

#include <map>

namespace
{
    // Виртуальные кошельки ботов (guid -> медяки).
    std::map<ObjectGuid::LowType, uint32> s_wallets;

    uint32 const BaseGoldPerKill = 5;  // базовое золото за моба
    uint32 const ItemDropChance = 8;   // шанс выпадения предмета, %

    // Попытка подобрать случайный предмет-экипировку.
    ItemTemplate const* RollRandomEquipment()
    {
        // До 25 попыток найти существующий предмет оружия/брони.
        for (int attempt = 0; attempt < 25; ++attempt)
        {
            uint32 entry = urand(1, 45000);
            ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(entry);
            if (tmpl && (tmpl->Class == ITEM_CLASS_WEAPON || tmpl->Class == ITEM_CLASS_ARMOR) && tmpl->ItemLevel > 0)
                return tmpl;
        }
        return nullptr;
    }

    bool IsBetterThanEquipped(Player* bot, ItemTemplate const* tmpl)
    {
        // Сравниваем по item level со слотом, куда предмет может встать.
        uint8 slot = tmpl->InventoryType == INVTYPE_WEAPON ||
                     tmpl->InventoryType == INVTYPE_WEAPONMAINHAND
                     ? uint8(EQUIPMENT_SLOT_MAINHAND)
                     : uint8(EQUIPMENT_SLOT_CHEST);

        if (tmpl->InventoryType == INVTYPE_RANGED || tmpl->InventoryType == INVTYPE_RANGEDRIGHT)
            slot = EQUIPMENT_SLOT_RANGED;

        if (Item const* equipped = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (ItemTemplate const* eqTmpl = equipped->GetTemplate())
                return tmpl->ItemLevel > eqTmpl->ItemLevel;

        return true; // слот пуст — предмет лучше.
    }
}

namespace AiBotsLoot
{
    void CollectFrom(Player* bot, Unit* victim)
    {
        if (!bot || !victim || !victim->IsCreature())
            return;

        ObjectGuid::LowType guid = bot->GetGUID().GetCounter();

        // Бонус золота от перков гильдии (Cash Flow, Bartering и др.).
        uint32 goldBonusPct = 100;
        if (bot->GetGuildId())
            goldBonusPct = AiBotsGuild::GetGoldBonusPct();

        // Золото пропорционально уровню жертвы.
        uint32 gold = (BaseGoldPerKill + victim->GetLevel() * urand(1, 3)) * goldBonusPct / 100;
        s_wallets[guid] += gold;

        // Часть золота — в казну гильдии (как Cash Flow).
        AiBotsGuild::DepositToVault(gold / 10);

        // Изредка с моба падает предмет.
        if (!roll_chance_i(ItemDropChance))
            return;

        ItemTemplate const* tmpl = RollRandomEquipment();
        if (!tmpl)
            return;

        if (IsBetterThanEquipped(bot, tmpl))
        {
            // Лучшая вещь — надеваем (через штатный механизм).
            ItemPosCountVec dest;
            if (bot->CanEquipItem(NULL_SLOT, dest, tmpl->ItemId, true))
            {
                if (Item* item = bot->EquipNewItem(dest.front(), tmpl->ItemId, true))
                {
                    bot->ApplyItemMods(item, item->GetSlot(), true);
                    LOG_DEBUG("server.loading", ">> mod-ai-bots: бот {} надел предмет {} (ilv {}).",
                        guid, tmpl->ItemId, tmpl->ItemLevel);
                    return;
                }
            }
        }

        // Иначе — «продаём» в виртуальный кошелёк.
        s_wallets[guid] += tmpl->SellPrice;
    }

    uint32 GetGold(ObjectGuid::LowType botGuid)
    {
        auto it = s_wallets.find(botGuid);
        return it != s_wallets.end() ? it->second : 0;
    }
}
