#include <base/system.h>
#include <engine/server/server.h>
#include <engine/shared/config.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/base_pvp/base_pvp.h>
#include <game/server/instagib/enums.h>
#include <game/server/player.h>

#include <game/server/gamecontext.h>

void CGameContext::RegisterInstagibCommands()
{
	// config chains
	Console()->Chain("sv_scorelimit", ConchainGameinfoUpdate, this);
	Console()->Chain("sv_timelimit", ConchainGameinfoUpdate, this);
	Console()->Chain("sv_grenade_ammo_regen", ConchainResetInstasettingTees, this);
	Console()->Chain("sv_spawn_weapons", ConchainSpawnWeapons, this);
	Console()->Chain("sv_tournament_chat_smart", ConchainSmartChat, this);
	Console()->Chain("sv_tournament_chat", ConchainTournamentChat, this);
	Console()->Chain("sv_zcatch_colors", ConchainZcatchColors, this);
	Console()->Chain("sv_spectator_votes", ConchainSpectatorVotes, this);
	Console()->Chain("sv_spectator_votes_sixup", ConchainSpectatorVotes, this);
	Console()->Chain("sv_display_score", ConchainDisplayScore, this);
	Console()->Chain("sv_only_wallshot_kills", ConchainOnlyWallshotKills, this);
	Console()->Chain("sv_allow_zoom", ConchainAllowZoom, this);
	Console()->Chain("sv_rollback", ConchainRollback, this);

	// generated undocumented chat commands
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	Console()->Register("rank_" #sql_name, "?r[player name]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaRank##name, this, "Shows the all time " #sql_name " rank of player name (your stats by default)");
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	Console()->Register("top5" #sql_name, "?i[rank to start with]", CFGFLAG_CHAT | CFGFLAG_SERVER, ConInstaTop##name, this, "Shows the all time best ranks by " #sql_name);
#include <game/server/instagib/sql_colums_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN

	// chat commands
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help) Console()->Register(name, params, flags, callback, userdata, help);
#include <game/server/instagib/chat_commands.h>
#undef CONSOLE_COMMAND

	// rcon commands
#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help) Console()->Register(name, params, flags, callback, userdata, help);
#include <game/server/instagib/rcon_commands.h>
#undef CONSOLE_COMMAND

	// generate callbacks to trigger insta settings update for all instagib configs
	// when one of the insta configs is changed
	// we update the checkboxes [x] in the vote menu
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Flags, Desc) \
	Console()->Chain(#ScriptName, ConchainInstaSettingsUpdate, this);
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) // only int checkboxes for now
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) // only int checkboxes for now
#include <engine/shared/variables_insta.h>
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_COL
#undef MACRO_CONFIG_STR
	Console()->Chain("sv_gametype", ConchainInstaSettingsUpdate, this);
}

void CGameContext::ConchainInstaSettingsUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->UpdateVoteCheckboxes();
	pSelf->RefreshVotes();
}

void CGameContext::ConchainGameinfoUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CGameContext *pSelf = (CGameContext *)pUserData;
		if(pSelf->m_pController)
			pSelf->m_pController->CheckGameInfo();
	}
}

void CGameContext::ConchainResetInstasettingTees(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pResult->NumArguments())
	{
		for(auto *pPlayer : pSelf->m_apPlayers)
		{
			if(!pPlayer)
				continue;
			CCharacter *pChr = pPlayer->GetCharacter();
			if(!pChr)
				continue;
			pChr->ResetInstaSettings();
		}
	}
}

void CGameContext::ConchainSpawnWeapons(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
	{
		CGameContext *pSelf = (CGameContext *)pUserData;
		if(pSelf->m_pController)
			pSelf->m_pController->UpdateSpawnWeapons();
	}
}

void CGameContext::ConchainSmartChat(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	if(!pResult->NumArguments())
		return;

	CGameContext *pSelf = (CGameContext *)pUserData;
	char aBuf[512];
	str_format(
		aBuf,
		sizeof(aBuf),
		"Warning: sv_tournament_chat is currently set to %d you might want to update that too.",
		pSelf->Config()->m_SvTournamentChat);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", aBuf);
}

void CGameContext::ConchainTournamentChat(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->Config()->m_SvTournamentChatSmart)
		return;

	pSelf->Console()->Print(
		IConsole::OUTPUT_LEVEL_STANDARD,
		"ddnet-insta",
		"Warning: this variable will be set automatically on round end because sv_tournament_chat_smart is active.");
}

void CGameContext::ConchainZcatchColors(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pSelf->m_pController)
		pSelf->m_pController->OnUpdateZcatchColorConfig();
}

void CGameContext::ConchainSpectatorVotes(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pSelf->m_pController)
		pSelf->m_pController->OnUpdateSpectatorVotesConfig();
}

void CGameContext::ConchainDisplayScore(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pResult->NumArguments() == 0)
	{
		pfnCallback(pResult, pCallbackUserData);
		return;
	}

	if(!str_to_display_score(pResult->GetString(0), &pSelf->m_DisplayScore))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' is not a valid display score pick one of those: " DISPLAY_SCORE_VALUES, pResult->GetString(0));
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", aBuf);
		return;
	}

	pfnCallback(pResult, pCallbackUserData);

	for(CPlayer *pPlayer : pSelf->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_DisplayScore = pSelf->m_DisplayScore;
	}
}

void CGameContext::ConchainOnlyWallshotKills(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pfnCallback(pResult, pCallbackUserData);

	if(pResult->NumArguments() == 0)
		return;

	if(g_Config.m_SvOnlyWallshotKills)
		pSelf->SendChat(-1, TEAM_ALL, "WARNING: only wallshots can kill");
	else
		pSelf->SendChat(-1, TEAM_ALL, "WARNING: wallshot is not needed anymore to kill");
}

void CGameContext::ConchainAllowZoom(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);

#ifdef CONF_ANTIBOT
	CGameContext *pSelf = (CGameContext *)pUserData;

	if(pResult->NumArguments() == 0)
		return;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "antibot sv_allow_zoom %d", g_Config.m_SvAllowZoom);
	pSelf->Console()->ExecuteLine(aBuf);
#endif
}

void CGameContext::ConchainRollback(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pfnCallback(pResult, pCallbackUserData);

	//Dont keep rollback enabled if server does not allow it

	if(!g_Config.m_SvRollback)
	{
		for(CPlayer *pPlayer : pSelf->m_apPlayers)
		{
			if(!pPlayer)
				continue;

			pPlayer->m_RollbackEnabled = false;
		}
	}
}
