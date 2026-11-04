/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotGear.cpp
 *
 * Наборы экипировки (itemEntry из стандартного DBC 3.3.5a) по классам:
 * воин/паладин — plate, разбойник/охотник/шаман — mail/leather,
 * жрец/маг/чернокнижник/друид — cloth/leather. Оружие по классу.
 */

#include "BotGear.h"

#include "Item.h"
#include "Player.h"

namespace
{
    // Plate: воин, паладин.
    BotGearSet const plateSet = { 16866, 16861, 16864, 16867, 16863, 16862, 16865, 16868, 21210, 0 };       // Might set +axe
    // Mail: охотник, шаман.
    BotGearSet const mailSet  = { 16865, 16861, 16863, 16864, 16862, 16866, 16867, 16868, 21214, 17069 };   // Beaststalker +bow
    // Leather: разбойник, друид.
    BotGearSet const leatherSet = { 16866, 16862, 16863, 16867, 16861, 16864, 16865, 16868, 21212, 0 };     // leather + dagger
    // Cloth: жрец, маг, чернокнижник.
    BotGearSet const clothSet = { 16865, 16861, 16862, 16864, 16863, 16866, 16867, 16868, 21211, 0 };       // cloth + staff
}

namespace AiBotsGear
{
    BotGearSet const& GetGearForClass(uint8 cls)
    {
        switch (cls)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:  return plateSet;
            case CLASS_HUNTER:
            case CLASS_SHAMAN:   return mailSet;
            case CLASS_ROGUE:
            case CLASS_DRUID:    return leatherSet;
            case CLASS_PRIEST:
            case CLASS_MAGE:
            case CLASS_WARLOCK:  return clothSet;
            default:             return clothSet;
        }
    }

    void RandomizeAppearance(uint8& skin, uint8& face, uint8& hairStyle,
                             uint8& hairColor, uint8& facialFeature, uint8& tattoos)
    {
        skin         = urand(0, 6);  // обычно 7-9 вариантов на расу
        face         = urand(0, 5);
        hairStyle    = urand(0, 10);
        hairColor    = urand(0, 8);
        facialFeature= urand(0, 5);  // усы/борода/серьги
        tattoos      = 0;
    }

    void EquipGearFor(Player* bot)
    {
        if (!bot)
            return;

        BotGearSet const& gear = GetGearForClass(bot->getClass());

        // Слоты: 0=голова 2=плечи 4=грудь 6=пояс 7=ноги 8=ступни 9=запястья 10=руки
        uint32 slots[] = { gear.head, gear.shoulders, gear.chest, gear.waist, gear.legs, gear.feet, gear.wrists, gear.hands };
        uint8 slotIds[] = { EQUIPMENT_SLOT_HEAD, EQUIPMENT_SLOT_SHOULDERS, EQUIPMENT_SLOT_CHEST, EQUIPMENT_SLOT_WAIST,
                            EQUIPMENT_SLOT_LEGS, EQUIPMENT_SLOT_FEET, EQUIPMENT_SLOT_WRISTS, EQUIPMENT_SLOT_HANDS };

        for (uint8 i = 0; i < 8; ++i)
        {
            if (slots[i] == 0 || bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slotIds[i]))
                continue;

            uint32 itemId = slots[i];
            ItemPosCountVec dest;
            if (bot->CanEquipItem(slotIds[i], dest, itemId, true))
            {
                if (Item* item = bot->EquipNewItem(dest.front(), itemId, true))
                    bot->ApplyItemMods(item, slotIds[i], true);
            }
        }

        // Оружие — правая рука.
        if (gear.mainHand && !bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
        {
            uint32 itemId = gear.mainHand;
            ItemPosCountVec dest;
            if (bot->CanEquipItem(EQUIPMENT_SLOT_MAINHAND, dest, itemId, true))
            {
                if (Item* item = bot->EquipNewItem(dest.front(), itemId, true))
                    bot->ApplyItemMods(item, EQUIPMENT_SLOT_MAINHAND, true);
            }
        }

        // Дальний бой (лук), если есть.
        if (gear.ranged && !bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED))
        {
            uint32 itemId = gear.ranged;
            ItemPosCountVec dest;
            if (bot->CanEquipItem(EQUIPMENT_SLOT_RANGED, dest, itemId, true))
            {
                if (Item* item = bot->EquipNewItem(dest.front(), itemId, true))
                    bot->ApplyItemMods(item, EQUIPMENT_SLOT_RANGED, true);
            }
        }
    }
}
