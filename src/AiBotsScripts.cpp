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
            { "add",    HandleBotAdd,    SEC_PLAYER, Console::No },
            { "remove", HandleBotRemove, SEC_PLAYER, Console::No },
            { "list",   HandleBotList,   SEC_PLAYER, Console::No },
            { "follow", HandleBotFollow, SEC_PLAYER, Console::No },
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
        // Автономные боты спавнятся при старте мира.
        sWorldBots->SpawnAll();
    }

    void OnUpdate(uint32 diff) override
    {
        sBotMgr->Update(diff);
        sWorldBots->Update(diff);
    }
};

void AddAiBotsScripts()
{
    new AiBotsCommandScript();
    new AiBotsPlayerScript();
    new AiBotsWorldScript();
}
