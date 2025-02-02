#include <base/math.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/strhelpers.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include "base_pvp.h"

void CGameControllerPvp::DoWarmup(int Seconds)
{
	CGameControllerDDRace::DoWarmup(Seconds);

	if(Seconds)
	{
		if(g_Config.m_SvTournamentChatSmart && !DetectedCasualRound())
		{
			g_Config.m_SvTournamentChat = 1;
			GameServer()->SendChat(-1, TEAM_ALL, "Spectators can no longer use public chat");
		}
	}
}

bool CGameControllerPvp::DetectedCasualRound()
{
	int NumAfks = 0;
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->m_IsCompetitiveAfk && pPlayer->GetTeam() != TEAM_SPECTATORS)
			NumAfks++;
	}

	int ActivePlayers = NumActivePlayers();
	bool FreeSlots = ActivePlayers < Server()->MaxClients() - g_Config.m_SvSpectatorSlots;

	// yes if someone leaves during a dm1 1on1
	// spectators can start chatting instantly
	// and that makes sense
	//
	// if a ctf game drops down to one player this is always
	// a casual round again
	if(ActivePlayers < 2)
		return true;

	// two people did not move in the last 6 seconds
	// and at least one team is missing players
	if(FreeSlots && NumAfks > 1)
		return true;

	// could also check:
	// - chat usage (trigger words)
	// - team switches
	// - vote calls
	// see https://github.com/ddnet-insta/ddnet-insta/issues/256

	return false;
}

void CGameControllerPvp::SmartChatTick()
{
	// do not compute this stuff every tick
	// waste of clock cycles
	if(Server()->Tick() % 10)
		return;

	if(DetectedCasualRound())
	{
		if(g_Config.m_SvTournamentChat)
		{
			SendChat(-1, TEAM_ALL, "Spectators can use public chat again. Because this is no longer a competitve round.");
			g_Config.m_SvTournamentChat = 0;
		}
	}
}

bool CGameControllerPvp::AllowPublicChat(const CPlayer *pPlayer)
{
	if(!g_Config.m_SvTournamentChat)
		return true;
	if(GameServer()->m_World.m_Paused && g_Config.m_SvTournamentChatSmart)
		return true;

	if(g_Config.m_SvTournamentChat == 1 && pPlayer->GetTeam() == TEAM_SPECTATORS)
		return false;
	else if(g_Config.m_SvTournamentChat == 2)
		return false;
	return true;
}

bool CGameControllerPvp::ParseChatCmd(char Prefix, int ClientId, const char *pCmdWithArgs)
{
#define MAX_ARG_LEN 256
	char aCmd[MAX_ARG_LEN];
	int i;
	for(i = 0; pCmdWithArgs[i] && i < MAX_ARG_LEN; i++)
	{
		if(pCmdWithArgs[i] == ' ')
			break;
		aCmd[i] = pCmdWithArgs[i];
	}
	aCmd[i] = '\0';

// max 16 args of 128 len each
#define MAX_ARGS 16

	// int RestOffset = m_pClient->m_ChatHelper.ChatCommandGetROffset(aCmd);
	int RestOffset = MAX_ARGS + 2; // TODO: add params with typed args: s,r,i

	char **ppArgs = new char *[MAX_ARGS];
	for(int x = 0; x < MAX_ARGS; ++x)
	{
		ppArgs[x] = new char[MAX_ARG_LEN];
		ppArgs[x][0] = '\0';
	}
	int NumArgs = 0;
	int k = 0;
	while(pCmdWithArgs[i])
	{
		if(k + 1 >= MAX_ARG_LEN)
		{
			dbg_msg("ddnet-insta", "ERROR: chat command has too long arg");
			break;
		}
		if(NumArgs + 1 >= MAX_ARGS)
		{
			dbg_msg("ddnet-insta", "ERROR: chat command has too many args");
			break;
		}
		if(pCmdWithArgs[i] == ' ')
		{
			// do not delimit on space
			// if we reached the r parameter
			if(NumArgs == RestOffset)
			{
				// strip spaces from the beggining
				// add spaces in the middle and end
				if(ppArgs[NumArgs][0])
				{
					ppArgs[NumArgs][k] = pCmdWithArgs[i];
					k++;
					i++;
					continue;
				}
			}
			else if(ppArgs[NumArgs][0])
			{
				ppArgs[NumArgs][k] = '\0';
				k = 0;
				NumArgs++;
			}
			i++;
			continue;
		}
		ppArgs[NumArgs][k] = pCmdWithArgs[i];
		k++;
		i++;
	}
	if(ppArgs[NumArgs][0])
	{
		ppArgs[NumArgs][k] = '\0';
		NumArgs++;
	}

	char aArgsStr[128];
	aArgsStr[0] = '\0';
	for(i = 0; i < NumArgs; i++)
	{
		if(aArgsStr[0] != '\0')
			str_append(aArgsStr, ", ", sizeof(aArgsStr));
		str_append(aArgsStr, ppArgs[i], sizeof(aArgsStr));
	}

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "got cmd '%s' with %d args: %s", aCmd, NumArgs, aArgsStr);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "bang-command", aBuf);

	bool match = OnBangCommand(ClientId, aCmd, NumArgs, (const char **)ppArgs);
	for(int x = 0; x < MAX_ARGS; ++x)
		delete[] ppArgs[x];
	delete[] ppArgs;
	return match;
}

// checks if it matches !1v1 !1vs1 and so on chat commands
// where 1 is the TeamSlots argument
static int Match1v1ChatCommand(const char *pCmd, int TeamSlots)
{
	const char aaVs[][16] = {"on", "n", "vs", "v"};
	for(const auto *pVs : aaVs)
	{
		char a1on1[32];
		str_format(a1on1, sizeof(a1on1), "%d%s%d", TeamSlots, pVs, TeamSlots);
		if(!str_comp_nocase(pCmd, a1on1))
			return true;
		str_format(a1on1, sizeof(a1on1), "%s%d", pVs, TeamSlots);
		if(!str_comp_nocase(pCmd, a1on1))
			return true;
	}
	return false;
}

bool CGameControllerPvp::OnBangCommand(int ClientId, const char *pCmd, int NumArgs, const char **ppArgs)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return false;
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;

	if(!g_Config.m_SvBangCommands)
	{
		SendChatTarget(ClientId, "This command is currently disabled.");
		return false;
	}

	if(!str_comp_nocase(pCmd, "set") || !str_comp_nocase(pCmd, "sett") || !str_comp_nocase(pCmd, "settings") || !str_comp_nocase(pCmd, "config"))
	{
		GameServer()->ShowCurrentInstagibConfigsMotd(ClientId, true);
		return true;
	}
	else if(!str_comp_nocase(pCmd, "gamestate"))
	{
		if(NumArgs > 0)
		{
			if(!str_comp_nocase(ppArgs[0], "on"))
				pPlayer->m_GameStateBroadcast = true;
			else if(!str_comp_nocase(ppArgs[0], "off"))
				pPlayer->m_GameStateBroadcast = false;
			else
				SendChatTarget(ClientId, "usage: !gamestate [on|off]");
		}
		else
			pPlayer->m_GameStateBroadcast = !pPlayer->m_GameStateBroadcast;
	}

	if(pPlayer->GetTeam() == TEAM_SPECTATORS && !g_Config.m_SvSpectatorVotes)
	{
		SendChatTarget(ClientId, "Spectators aren't allowed to vote.");
		return false;
	}

	if(g_Config.m_SvBangCommands < 2)
	{
		SendChatTarget(ClientId, "Chat votes are disabled please use the vote menu.");
		return false;
	}

	int SetSlots = -1;
	const int aSlotCommandValues[] = {1, 2, 3, 4, 5, 6, 7, 8, 32};
	for(int Slots : aSlotCommandValues)
	{
		if(Match1v1ChatCommand(pCmd, Slots))
		{
			SetSlots = Slots;
			break;
		}
	}

	if(SetSlots != -1)
	{
		char aCmd[512];
		str_format(aCmd, sizeof(aCmd), "sv_spectator_slots %d", MAX_CLIENTS - (SetSlots * 2));
		char aDesc[512];
		str_format(aDesc, sizeof(aDesc), "%dvs%d", SetSlots, SetSlots);
		GameServer()->BangCommandVote(ClientId, aCmd, aDesc);
	}
	else if(!str_comp_nocase(pCmd, "restart") || !str_comp_nocase(pCmd, "reload"))
	{
		int Seconds = NumArgs > 0 ? atoi(ppArgs[0]) : 10;
		Seconds = clamp(Seconds, 1, 200);
		char aCmd[512];
		str_format(aCmd, sizeof(aCmd), "restart %d", Seconds);
		char aDesc[512];
		str_format(aDesc, sizeof(aDesc), "restart %d", Seconds);
		GameServer()->BangCommandVote(ClientId, aCmd, aDesc);
	}
	else if(!str_comp_nocase(pCmd, "ready") || !str_comp_nocase(pCmd, "pause"))
	{
		GameServer()->m_pController->OnPlayerReadyChange(pPlayer);
	}
	else if(!str_comp_nocase(pCmd, "shuffle"))
	{
		GameServer()->ComCallShuffleVote(ClientId);
	}
	else if(!str_comp_nocase(pCmd, "swap"))
	{
		GameServer()->ComCallSwapTeamsVote(ClientId);
	}
	else if(!str_comp_nocase(pCmd, "swap_random"))
	{
		GameServer()->ComCallSwapTeamsRandomVote(ClientId);
	}
	else
	{
		SendChatTarget(ClientId, "Unknown command. Commands: !restart, !ready, !shuffle, !swap, !swap_random, !1on1, !settings, !gamestate");
		return false;
	}
	return true;
}

bool CGameControllerPvp::IsChatBlocked(const CNetMsg_Cl_Say *pMsg, int Length, int Team, CPlayer *pPlayer) const
{
	int ClientId = pPlayer->GetCid();

	if(!g_Config.m_SvRequireChatFlagToChat)
		return false;

	// always allow sending chat commands
	// yes this enables /me spam but nobody knows
	if(pMsg->m_pMessage[0] == '/')
		return false;

	// spammers do not get pinged
	// but if real players get greeted they
	// should be able to respond instantly
	if(pPlayer->m_GotPingedInChat)
		return false;

	// all kinds of verifications such as
	// joining a few seconds after the server was empty
	// or being on the server before a map change
	if(pPlayer->m_VerifiedForChat)
		return false;

	// spectators can not send the playerflag chatting
	// legit chat binds also do not send the playerflag chatting
	// so after 20 seconds we allow everyone to use the chat
	// to cover those cases.
	// It should still filter out the reconnecting spam bots.
	int SecondsConnected = (time_get() - pPlayer->m_JoinTime) / time_freq();
	int SecondsUntilAllowed = maximum(0, 20 - SecondsConnected);
	if(SecondsUntilAllowed == 0)
		return false;

	// writing "hi" in less than 2 ticks is sus
	int ChatTicksNeeded = 2;

	// writing "hello world" in less than 20 ticks is sus
	if(Length > 10)
		ChatTicksNeeded = 20;

	if(pPlayer->m_TicksSpentChatting < ChatTicksNeeded)
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "You are not allowed to use the chat yet. Please wait %d seconds.", SecondsUntilAllowed);
		SendChatTarget(ClientId, aBuf);
		return true;
	}

	return false;
}

bool CGameControllerPvp::OnChatMessage(const CNetMsg_Cl_Say *pMsg, int Length, int &Team, CPlayer *pPlayer)
{
	int ClientId = pPlayer->GetCid();

	if(IsChatBlocked(pMsg, Length, Team, pPlayer))
		return true;

	if(pMsg->m_Team || !AllowPublicChat(pPlayer))
		Team = ((pPlayer->GetTeam() == TEAM_SPECTATORS) ? TEAM_SPECTATORS : pPlayer->GetTeam()); // ddnet-insta
	else
		Team = TEAM_ALL;

	// ddnet-insta warn on ping if cant respond
	if(pMsg->m_pMessage[0] != '/')
	{
		for(CPlayer *pPingedPlayer : GameServer()->m_apPlayers)
		{
			if(!pPingedPlayer)
				continue;
			if(!str_find_nocase(pMsg->m_pMessage, Server()->ClientName(pPingedPlayer->GetCid())))
				continue;

			if(
				Team == TEAM_ALL &&
				pPingedPlayer->GetTeam() == TEAM_SPECTATORS &&
				pPlayer->GetTeam() != TEAM_SPECTATORS &&
				!AllowPublicChat(pPingedPlayer))
			{
				char aChatText[256];
				str_format(aChatText, sizeof(aChatText), "Warning: '%s' got pinged in chat but can not respond", Server()->ClientName(pPingedPlayer->GetCid()));
				GameServer()->SendChat(-1, TEAM_ALL, aChatText);
				GameServer()->SendChat(-1, TEAM_ALL, "turn off tournament chat or make sure there are enough in game slots");
			}

			pPingedPlayer->m_GotPingedInChat = true;
		}
	}

	if(g_Config.m_SvStopAndGoChat && pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		if(!str_comp_nocase(pMsg->m_pMessage, "stop") || !str_comp_nocase(pMsg->m_pMessage, "pause"))
		{
			SetGameState(IGS_GAME_PAUSED, TIMER_INFINITE);
			GameServer()->SendGameMsg(protocol7::GAMEMSG_GAME_PAUSED, pPlayer->GetCid(), -1);
		}
		else if(!str_comp_nocase(pMsg->m_pMessage, "start") || !str_comp_nocase(pMsg->m_pMessage, "go"))
		{
			if(GameState() == IGS_GAME_PAUSED)
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "'%s' started the game", Server()->ClientName(ClientId));
				GameServer()->SendChat(-1, TEAM_ALL, aBuf);
				SetGameState(IGS_GAME_PAUSED, 0);
			}
		}
	}

	// ddnet-insta fine grained chat spam control
	if(pMsg->m_pMessage[0] != '/')
	{
		bool RateLimit = false;
		if(g_Config.m_SvChatRatelimitSpectators && pPlayer->GetTeam() == TEAM_SPECTATORS)
		{
			if(g_Config.m_SvChatRatelimitDebug)
				dbg_msg("ratelimit", "m_SvChatRatelimitSpectators %s", pMsg->m_pMessage);
			RateLimit = true;
		}
		if(g_Config.m_SvChatRatelimitPublicChat && Team == TEAM_ALL)
		{
			if(g_Config.m_SvChatRatelimitDebug)
				dbg_msg("ratelimit", "m_SvChatRatelimitPublicChat %s", pMsg->m_pMessage);
			RateLimit = true;
		}
		if(g_Config.m_SvChatRatelimitLongMessages && Length >= 12)
		{
			if(g_Config.m_SvChatRatelimitDebug)
				dbg_msg("ratelimit", "m_SvChatRatelimitLongMessages %s", pMsg->m_pMessage);
			RateLimit = true;
		}
		if(g_Config.m_SvChatRatelimitNonCalls)
		{
			// grep -r teamchat | cut -d: -f8- | sort | uniq -c | sort -nr
			const char aaCalls[][24] = {
				"help",
				"mid",
				"top",
				"bot",
				"Back!!!",
				"back",
				"Enemy Base!",
				"enemy_base",
				"mid!",
				"TOP !",
				"Mid",
				"back! / help!",
				"top!",
				"Top",
				"MID !",
				"BACK !",
				"bottom",
				"Ready!!",
				"back!",
				"BOT !",
				"help",
				"Back",
				"ENNEMIE BASE !",
				"Backkk",
				"back!!",
				"left",
				"READY !",
				"Double Attack!!",
				"Mid!",
				"Help! / Back!",
				"bot!",
				"Back!",
				"Top!",
				"🅷🅴🅻🅿",
				"right",
				"enemy base",
				"b",
				"Bottom",
				"Ready!",
				"middle",
				"bottom!",
				"Bot!",
				"Bot",
				"right down",
				"pos?",
				"atk",
				"ready!",
				"DoubleAttack!",
				"Our base!!!",
				"HELP !",
				"def",
				"pos",
				"Back!Help!",
				"right top",
				"pos ?",
				"Take weapons",
				"go",
				"enemy base!",
				"deff",
				"BACK!!!",
				"safe",
			};
			bool IsCall = false;
			for(const char *aCall : aaCalls)
			{
				if(!str_comp_nocase(aCall, pMsg->m_pMessage))
				{
					IsCall = true;
					break;
				}
			}
			if(!IsCall)
			{
				if(g_Config.m_SvChatRatelimitDebug)
					dbg_msg("ratelimit", "m_SvChatRatelimitNonCalls %s", pMsg->m_pMessage);
				RateLimit = true;
			}
		}
		if(g_Config.m_SvChatRatelimitSpam)
		{
			char aaSpams[][24] = {
				"discord.gg",
				"fuck",
				"idiot",
				"http://",
				"https://",
				"www.",
				".com",
				".de",
				".io"};
			for(const char *aSpam : aaSpams)
			{
				if(str_find_nocase(aSpam, pMsg->m_pMessage))
				{
					if(g_Config.m_SvChatRatelimitDebug)
						dbg_msg("ratelimit", "m_SvChatRatelimitSpam (bad word) %s", pMsg->m_pMessage);
					RateLimit = true;
					break;
				}
			}
			if(str_contains_ip(pMsg->m_pMessage))
			{
				if(g_Config.m_SvChatRatelimitDebug)
					dbg_msg("ratelimit", "m_SvChatRatelimitSpam (ip) %s", pMsg->m_pMessage);
				RateLimit = true;
			}
		}
		if(RateLimit && pPlayer->m_LastChat && pPlayer->m_LastChat + Server()->TickSpeed() * ((31 + Length) / 32) > Server()->Tick())
			return true;
	}

	// ddnet-insta bang commands
	// allow sending ! to chat or !!
	// swallow all other ! prefixed chat messages
	if(pMsg->m_pMessage[0] == '!' && pMsg->m_pMessage[1] && pMsg->m_pMessage[1] != '!')
	{
		ParseChatCmd('!', ClientId, pMsg->m_pMessage + 1);
		return true;
	}

	return false;
}
