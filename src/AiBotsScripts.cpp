/*
 * mod-ai-bots — модуль AI-ботов для AzerothCore
 * Файл: src/AiBotsScripts.cpp
 *
 * Точка входа модуля: команды .bot, автоспавн при логине,
 * автономные боты в мире (WorldBots), обновление AI в WorldScript-тикере,
 * загрузка конфига.
 */

#include "BotAI.h"
#include "BotMgr.h"
#include "Config_Bots.h"
#include "WorldBots.h"
#include "BotGroups.h"
#include "BotSocial.h"
#include "BotTerritory.h"
#include "BotVendors.h"
#include "BotLoot.h"
#include "BotGuild.h"
#include "BotPets.h"
#include "BotSiege.h"
#include "Chat.h"
#include "CommandScript.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldScript.h"

using namespace Acore::ChatCommands;

class AiBotsCommandScript : public CommandScript
{
public:
    AiBotsCommandScript() : CommandScript("AiBotsCommandScript") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable botCommandTable =
        {
            { "add",       HandleBotAdd,       SEC_PLAYER, Console::No },
            { "remove",    HandleBotRemove,    SEC_PLAYER, Console::No },
            { "list",      HandleBotList,      SEC_PLAYER, Console::No },
            { "follow",    HandleBotFollow,    SEC_PLAYER, Console::No },
            { "shop",      HandleBotShop,      SEC_PLAYER, Console::No },
            { "buy",       HandleBotBuy,       SEC_PLAYER, Console::No },
            { "gold",      HandleBotGold,      SEC_PLAYER, Console::No },
            { "pet",       HandleBotPet,       SEC_PLAYER, Console::No },
            { "groups",    HandleBotGroups,    SEC_PLAYER, Console::No },
            { "territory", HandleBotTerritory, SEC_PLAYER, Console::No },
            { "guild",     HandleBotGuild,     SEC_PLAYER, Console::No },
            { "siege",     HandleBotSiege,     SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "bot", botCommandTable },
        };

        return commandTable;
    }

    static bool HandleBotAdd(ChatHandler* handler, Optional<std::string> args)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        if (!AiBotsConfig::IsEnabled())
        {
            handler->PSendSysMessage("[AI-Bot] Модуль выключен (AiBots.Enable = 0).");
            return true;
        }

        uint8 race = 0, cls = 0;
        if (args && !args->empty())
        {
            // формат: .bot add <race>:<class>, например .bot add 1:1
            auto pos = args->find(':');
            if (pos != std::string::npos)
            {
                try
                {
                    race = static_cast<uint8>(std::stoi(args->substr(0, pos)));
                    cls  = static_cast<uint8>(std::stoi(args->substr(pos + 1)));
                }
                catch (std::exception const&)
                {
                    handler->PSendSysMessage("[AI-Bot] Неверный формат. Использование: .bot add [race:class]");
                    return true;
                }
            }
        }

        sBotMgr->CreateBotForPlayer(player, race, cls);
        return true;
    }

    static bool HandleBotRemove(ChatHandler* handler, std::string const& botName)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        if (!AiBotsConfig::IsEnabled())
        {
            handler->PSendSysMessage("[AI-Bot] Модуль выключен (AiBots.Enable = 0).");
            return true;
        }

        if (botName.empty())
        {
            sBotMgr->RemoveAllBots(player);
            handler->PSendSysMessage("[AI-Bot] Все боты удалены.");
        }
        else
            sBotMgr->RemoveBot(player, botName);

        return true;
    }

    static bool HandleBotList(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        auto const& bots = sBotMgr->GetBotsOfOwner(player->GetGUID().GetCounter());
        if (bots.empty())
        {
            handler->PSendSysMessage("[AI-Bot] У вас нет ботов. Создайте: .bot add [race:class]");
            return true;
        }

        handler->PSendSysMessage("[AI-Bot] Ваши боты:");
        for (auto const& bot : bots)
            handler->PSendSysMessage("  - {} (раса {}, класс {}, уровень {}, в мире: {})",
                bot.name, bot.race, bot.cls, bot.level, bot.inWorld ? "да" : "нет");

        return true;
    }

    static bool HandleBotFollow(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        auto const& bots = sBotMgr->GetBotsOfOwner(player->GetGUID().GetCounter());
        for (auto const& botData : bots)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(botData.guid)))
            {
                bot->GetMotionMaster()->MoveFollow(player, AiBotsConfig::GetFollowDistance(), 0.0f);
                handler->PSendSysMessage("[AI-Bot] '{}' следует за вами.", botData.name);
            }
        }

        return true;
    }

    // Найти ближайшего бота-торговца в радиусе 15 ярдов.
    static Player* FindNearbyVendor(Player* player)
    {
        auto const& allBots = sBotMgr->GetAllBots();
        Player* best = nullptr;
        float bestDist = 15.0f;
        for (auto const& pair : allBots)
        {
            if (!AiBotsVendors::GetStock(pair.first))
                continue;
            if (Player* vendor = ObjectAccessor::FindPlayer(ObjectGuid::Create<HighGuid::Player>(pair.first)))
            {
                if (!vendor->IsInWorld())
                    continue;
                float dist = player->GetDistance(vendor);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    best = vendor;
                }
            }
        }
        return best;
    }

    static bool HandleBotShop(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player || !AiBotsConfig::IsVendorsEnabled())
            return false;

        Player* vendor = FindNearbyVendor(player);
        if (!vendor)
        {
            handler->PSendSysMessage("[AI-Bot] Рядом нет торговца (радиус 15 ярдов).");
            return true;
        }

        auto const* stock = AiBotsVendors::GetStock(vendor->GetGUID().GetCounter());
        if (!stock || stock->empty())
        {
            handler->PSendSysMessage("[AI-Bot] У торговца '{}' нет товаров.", vendor->GetName());
            return true;
        }

        handler->PSendSysMessage("[AI-Bot] Товары торговца '{}':", vendor->GetName());
        uint32 index = 1;
        for (auto const& item : *stock)
            handler->PSendSysMessage("  {}. itemId={} цена={}м запас={}",
                index++, item.itemId, item.price, item.stock == 0 ? "бесконечный" : std::to_string(item.stock));

        handler->PSendSysMessage("Покупка: .bot buy <номер>");
        return true;
    }

    static bool HandleBotBuy(ChatHandler* handler, uint32 itemIndex)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player || !AiBotsConfig::IsVendorsEnabled())
            return false;

        Player* vendor = FindNearbyVendor(player);
        if (!vendor)
        {
            handler->PSendSysMessage("[AI-Bot] Рядом нет торговца (радиус 15 ярдов).");
            return true;
        }

        // Индексы для игрока начинаются с 1.
        if (itemIndex == 0)
        {
            handler->PSendSysMessage("[AI-Bot] Укажите номер товара (см. .bot shop).");
            return true;
        }

        std::string error = AiBotsVendors::Buy(player, vendor->GetGUID().GetCounter(), itemIndex - 1);
        if (error.empty())
            handler->PSendSysMessage("[AI-Bot] Покупка совершена у '{}'.", vendor->GetName());
        else
            handler->PSendSysMessage("[AI-Bot] {}.", error);

        return true;
    }

    // Команда .bot gold — показать виртуальные кошельки ботов игрока.
    // Лут собирается автоматически (BotLoot), но не имел пользовательского интерфейса.
    static bool HandleBotGold(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        auto const& bots = sBotMgr->GetBotsOfOwner(player->GetGUID().GetCounter());
        if (bots.empty())
        {
            handler->PSendSysMessage("[AI-Bot] У вас нет ботов. Создайте: .bot add [race:class]");
            return true;
        }

        handler->PSendSysMessage("[AI-Bot] Кошельки ваших ботов:");
        for (auto const& bot : bots)
        {
            uint32 copper = AiBotsLoot::GetGold(bot.guid);
            handler->PSendSysMessage("  - {}: {}з {}с {}м",
                bot.name, copper / 10000, (copper / 100) % 100, copper % 100);
        }

        return true;
    }

    // Команда .bot pet — призвать питомца ботам-охотникам/чернокнижникам.
    // Подсистема BotPets была полностью автоматической без ручного управления.
    static bool HandleBotPet(ChatHandler* handler)
    {
        Player* player = handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
        if (!player)
            return false;

        if (!AiBotsConfig::IsPetsEnabled())
        {
            handler->PSendSysMessage("[AI-Bot] Питомцы выключены (AiBots.Pets.Enable = 0).");
            return true;
        }

        uint32 summoned = 0;
        for (auto const& bot : sBotMgr->GetBotsOfOwner(player->GetGUID().GetCounter()))
        {
            if (!bot.inWorld || !AiBotsPets::ClassHasPet(bot.cls))
                continue;

            Player* botPlayer = sBotMgr->GetBotPlayer(bot.guid);
            if (!botPlayer || !botPlayer->IsInWorld())
                continue;

            if (AiBotsPets::SummonPetFor(botPlayer))
                ++summoned;
        }

        if (summoned > 0)
            handler->PSendSysMessage("[AI-Bot] Призвано питомцев: {}.", summoned);
        else
            handler->PSendSysMessage("[AI-Bot] Нет ботов в мире с классом питомца (охотник/чернокнижник).");

        return true;
    }

    // Команда .bot groups — список групп автономных ботов.
    // API групп (BotGroups) был доступен только внутренним подсистемам.
    static bool HandleBotGroups(ChatHandler* handler)
    {
        if (!AiBotsConfig::IsGroupsEnabled())
        {
            handler->PSendSysMessage("[AI-Bot] Группы ботов выключены.");
            return true;
        }

        // Собираем уникальные группы: группируем боты по guid лидера.
        std::map<ObjectGuid::LowType, std::vector<std::string>> groupsByLeader;
        for (auto const& pair : sBotMgr->GetAllBots())
        {
            auto const* group = AiBotsGroups::GetGroupOf(pair.first);
            if (!group)
                continue;

            Player* member = sBotMgr->GetBotPlayer(pair.first);
            if (!member || !member->IsInWorld())
                continue;

            groupsByLeader[group->leaderGuid].push_back(member->GetName());
        }

        if (groupsByLeader.empty())
        {
            handler->PSendSysMessage("[AI-Bot] Сейчас нет активных групп ботов.");
            return true;
        }

        handler->PSendSysMessage("[AI-Bot] Группы ботов:");
        for (auto const& entry : groupsByLeader)
        {
            Player* leader = sBotMgr->GetBotPlayer(entry.first);
            std::string leaderName = leader && leader->IsInWorld()
                ? leader->GetName() : "<нет в мире>";

            handler->PSendSysMessage("  Лидер '{}', участников {}:",
                leaderName, entry.second.size());
            for (auto const& memberName : entry.second)
                if (memberName != leaderName)
                    handler->PSendSysMessage("    - {}", memberName);
        }

        return true;
    }

    static bool HandleBotTerritory(ChatHandler* handler)
    {
        if (!AiBotsConfig::IsTerritoryEnabled())
        {
            handler->PSendSysMessage("[AI-Bot] Система территорий выключена.");
            return true;
        }

        handler->PSendSysMessage("[AI-Bot] Территории:");
        for (auto const& zone : AiBotsTerritory::GetAll())
            handler->PSendSysMessage("  - {}: {}", zone.name, zone.owner == TEAM_HORDE ? "Орда" : "Альянс");

        return true;
    }

    static bool HandleBotGuild(ChatHandler* handler)
    {
        if (!AiBotsConfig::IsGuildEnabled())
        {
            handler->PSendSysMessage("[AI-Bot] Гильдия ботов выключена.");
            return true;
        }

        // Гильдии отдельные для каждой фракции — выводим обе.
        TeamId const teams[] = { TEAM_ALLIANCE, TEAM_HORDE };
        for (TeamId team : teams)
        {
            handler->PSendSysMessage("[AI-Bot] Гильдия '{}' ({}):",
                AiBotsGuild::GetGuildNameFor(team),
                team == TEAM_HORDE ? "Орда" : "Альянс");
            handler->PSendSysMessage("  Уровень: {}/{}",
                AiBotsGuild::GetLevel(team), AiBotsConfig::GetGuildMaxLevel());
            handler->PSendSysMessage("  Опыт: {}/{}",
                AiBotsGuild::GetXp(team), AiBotsGuild::GetXpForNextLevel(team));
            uint32 vault = AiBotsGuild::GetVault(team);
            handler->PSendSysMessage("  Казна: {}з {}с {}м",
                vault / 10000, (vault / 100) % 100, vault % 100);
            handler->PSendSysMessage("  Состав: {} участников", AiBotsGuild::GetMemberCount(team));

            uint32 xp = AiBotsGuild::GetXp(team);
            uint32 xpNext = AiBotsGuild::GetXpForNextLevel(team);
            uint32 pct = xpNext ? (xp * 100) / xpNext : 0;
            handler->PSendSysMessage("  Прогресс: {}%", pct);
        }

        return true;
    }

    static bool HandleBotSiege(ChatHandler* handler)
    {
        if (!AiBotsConfig::IsSiegeEnabled())
        {
            handler->PSendSysMessage("[AI-Bot] Осады выключены.");
            return true;
        }

        if (AiBotsSiege::IsSiegeActive())
            handler->PSendSysMessage("[AI-Bot] Осада идёт прямо сейчас!");
        else
            handler->PSendSysMessage("[AI-Bot] Осада не активна. Следующая — по расписанию ({}-{} мин).",
                AiBotsConfig::GetSiegeIntervalMin(), AiBotsConfig::GetSiegeIntervalMax());

        handler->PSendSysMessage("[AI-Bot] Цели осад:");
        for (auto const& target : AiBotsSiege::GetTargets())
            handler->PSendSysMessage("  - {} ({})", target.name,
                target.faction == TEAM_HORDE ? "Орда" : "Альянс");

        return true;
    }
};

class AiBotsPlayerScript : public PlayerScript
{
public:
    AiBotsPlayerScript() : PlayerScript("AiBotsPlayerScript") {}

    void OnLogin(Player* player) override
    {
        sBotMgr->HandlePlayerLogin(player);
    }

    void OnLogout(Player* player) override
    {
        sBotMgr->HandlePlayerLogout(player);
    }
};

class AiBotsWorldScript : public WorldScript
{
public:
    AiBotsWorldScript() : WorldScript("AiBotsWorldScript") {}

    void OnBeforeConfigLoad(bool reload) override
    {
        AiBotsConfig::Load();
    }

    void OnStartup() override
    {
        // Автономные боты, торговцы, территории и осады при старте мира.
        sWorldBots->SpawnAll();
        AiBotsVendors::SpawnAll();
        AiBotsTerritory::Init();
        AiBotsSiege::Init();
    }

    void OnUpdate(uint32 diff) override
    {
        sBotMgr->Update(diff);
        sWorldBots->Update(diff);
        AiBotsGroups::Update(diff);
        AiBotsSocial::Update(diff);
        AiBotsTerritory::Update(diff);
        AiBotsVendors::Update(diff);
        AiBotsGuild::Update(diff);
        AiBotsPets::Update(diff);
        AiBotsSiege::Update(diff);
    }
};

void AddAiBotsScripts()
{
    new AiBotsCommandScript();
    new AiBotsPlayerScript();
    new AiBotsWorldScript();
}
