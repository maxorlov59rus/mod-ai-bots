/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/BotVendors.cpp
 *
 * Торговцы — отдельные автономные боты, помеченные в БД
 * (таблица mod_ai_bots_vendors: botName + предмет/цена/запас).
 * Товары хранятся в памяти, покупка — через GameMail-механику
 * не используется: предмет выдаётся напрямую в сумку покупателя.
 */

#include "BotVendors.h"
#include "BotMgr.h"
#include "Config_Bots.h"

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"

#include <map>

namespace
{
    // guid бота-торговца -> список товаров
    std::map<ObjectGuid::LowType, std::vector<BotVendorItem>> s_stocks;
    std::vector<ObjectGuid::LowType> s_vendorGuids;
}

namespace AiBotsVendors
{
    void SpawnAll()
    {
        if (!AiBotsConfig::IsVendorsEnabled())
            return;

        // Загружаем товары из БД.
        QueryResult result = WorldDatabase.Query(
            "SELECT `botName`, `itemId`, `price`, `stock` FROM `mod_ai_bots_vendors`");
        if (!result)
        {
            LOG_WARN("server.loading", ">> mod-ai-bots: таблица mod_ai_bots_vendors пуста или отсутствует — торговцы не имеют товаров.");
            return;
        }

        // Собираем товары по именам ботов.
        std::map<std::string, std::vector<BotVendorItem>> byName;
        do
        {
            Field* fields = result->Fetch();
            byName[fields[0].Get<std::string>()].push_back(
                { fields[1].Get<uint32>(), fields[2].Get<uint32>(), fields[3].Get<uint32>() });
        } while (result->NextRow());

        // Спавним Count торговцев, каждому — имя из таблицы (порядок ключей).
        uint32 spawned = 0;
        for (auto const& pair : byName)
        {
            if (spawned >= AiBotsConfig::GetVendorsCount())
                break;

            // Создаём бота-торговца через общий механизм (уровень 80, стоит на месте).
            s_stocks[0] = pair.second; // stocks привязываются при логине бота ниже

            // Ищем бота с таким именем в БД персонажей (CharacterCache).
            if (CharacterCacheEntry const* entry = sCharacterCache->GetCharacterCacheByName(pair.first))
            {
                ObjectGuid::LowType guid = entry->Guid.GetCounter();
                s_stocks[guid] = pair.second;
                s_vendorGuids.push_back(guid);
                LOG_INFO("server.loading", ">> mod-ai-bots: торговец '{}' (guid {}) с {} товарами.",
                    pair.first, guid, pair.second.size());
            }

            ++spawned;
        }
    }

    void Update(uint32 /*diff*/)
    {
        // Пока без логики — расширение на будущее.
    }

    std::vector<BotVendorItem> const* GetStock(ObjectGuid::LowType botGuid)
    {
        auto it = s_stocks.find(botGuid);
        return it != s_stocks.end() ? &it->second : nullptr;
    }

    std::string Buy(Player* buyer, ObjectGuid::LowType vendorGuid, uint32 itemIndex)
    {
        auto it = s_stocks.find(vendorGuid);
        if (it == s_stocks.end() || itemIndex >= it->second.size())
            return "Торговец не найден или неверный номер товара.";

        BotVendorItem& item = it->second[itemIndex];

        // stock==0 трактуем как бесконечный запас — проверка не требуется,
        // ограниченный запас уменьшается после покупки.
        if (!buyer || buyer->GetMoney() < item.price)
            return "Недостаточно золота.";

        ItemTemplate const* tmpl = sObjectMgr->GetItemTemplate(item.itemId);
        if (!tmpl)
            return "Предмет не существует.";

        // Списание золота и выдача предмета.
        buyer->ModifyMoney(-static_cast<int32>(item.price));

        ItemPosCountVec dest;
        if (buyer->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemId, 1) == EQUIP_ERR_OK)
        {
            Item* stored = buyer->StoreNewItem(dest, item.itemId, true);
            if (stored)
                buyer->SendNewItem(stored, 1, true, false, true);
        }
        else
        {
            buyer->ModifyMoney(static_cast<int32>(item.price));
            return "Нет места в сумке.";
        }

        // Уменьшаем запас, если он ограничен.
        if (item.stock > 0)
            --item.stock;

        return ""; // успех
    }
}
