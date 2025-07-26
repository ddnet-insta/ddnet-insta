#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/enums.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include <game/server/gamecontext.h>

void CGameContext::ConCreditsGctf(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	static constexpr const char *CREDITS[] = {
		"DDNet-insta written by ChillerDragon",
		"https://github.com/ddnet-insta/ddnet-insta/",
		"Thanks to AssassinTee, Cuube, Anime-pdf, M0REKZ",
		"JSaurusRex, jxsl13, lukure, ByFox and zhn",
		"based on ddnet see /credits_ddnet",
	};
	for(const char *pLine : CREDITS)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameContext::ConReadyChange(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	pSelf->m_pController->OnPlayerReadyChange(pPlayer);
}

void CGameContext::ConInstaSwap(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pSelf->IsChatCmdAllowed(pResult->m_ClientId))
		return;

	pSelf->ComCallSwapTeamsVote(pResult->m_ClientId);
}

void CGameContext::ConInstaSwapRandom(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pSelf->IsChatCmdAllowed(pResult->m_ClientId))
		return;

	pSelf->ComCallSwapTeamsRandomVote(pResult->m_ClientId);
}

void CGameContext::ConInstaShuffle(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pSelf->IsChatCmdAllowed(pResult->m_ClientId))
		return;

	pSelf->ComCallShuffleVote(pResult->m_ClientId);
}

void CGameContext::ConInstaDrop(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	int ClientId = pResult->m_ClientId;
	if(pResult->NumArguments() != 1 || str_comp_nocase(pResult->GetString(0), "flag"))
	{
		pSelf->SendChatTarget(ClientId, "Did you mean '/drop flag'?");
		return;
	}

	pSelf->ComDropFlag(pResult->m_ClientId);
}

void CGameContext::ConRankCmdlist(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConRank(pResult, pUserData);
		return;
	}

	pSelf->SendChatTarget(pResult->m_ClientId, "~~~ ddnet-insta rank commands");
	pSelf->SendChatTarget(pResult->m_ClientId, "~ /rank_kills, /rank_spree");
	pSelf->SendChatTarget(pResult->m_ClientId, "~ /rank_wins, /rank_win_points");
	pSelf->SendChatTarget(pResult->m_ClientId, "~ /points");
	pSelf->SendChatTarget(pResult->m_ClientId, "~ /stats, /statsall");
	if(pSelf->m_pController->GameFlags() & GAMEFLAG_FLAGS)
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "~ /rank_caps, /rank_flags");
	}
	if(pSelf->m_pController->IsFngGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "~ /multis");
	}
	pSelf->SendChatTarget(pResult->m_ClientId, "~ see also /top5 for a list of top commands");
}

void CGameContext::ConTopCmdlist(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConTop(pResult, pUserData);
		return;
	}

	pSelf->SendChatTarget(pResult->m_ClientId, "~~~ ddnet-insta top commands");
	pSelf->SendChatTarget(pResult->m_ClientId, "~ /top5kills, /top5spree");
	pSelf->SendChatTarget(pResult->m_ClientId, "~ /top5wins, /top5win_points");
	pSelf->SendChatTarget(pResult->m_ClientId, "~ /top5points");
	if(pSelf->m_pController->GameFlags() & GAMEFLAG_FLAGS)
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "~ /top5caps, /top5flags");
	}
	if(pSelf->m_pController->IsFngGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "~ /top5spikes");
	}
	pSelf->SendChatTarget(pResult->m_ClientId, "~ see also /rank for a list of rank commands");
}

void CGameContext::ConStatsRound(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	char aBuf[512];
	char aReason[512];
	aReason[0] = 0;

	if(!pSelf->m_pController->IsStatTrack(aReason, sizeof(aReason)))
	{
		str_format(aBuf, sizeof(aBuf), "!!! stats are currently not tracked (%s)", aReason);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	int TargetId = pSelf->m_pController->GetCidByName(pName);
	if(TargetId < 0 || TargetId >= MAX_CLIENTS)
		return;
	const CPlayer *pPlayer = pSelf->m_apPlayers[TargetId];
	CPlayer *pRequestingPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pRequestingPlayer)
		return;

	char aUntrackedOrAccuracy[512];
	str_format(aBuf, sizeof(aBuf), "~~~ round stats for '%s' (see also /statsall)", pName);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Points: %d", pPlayer->m_Stats.m_Points);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);

	aUntrackedOrAccuracy[0] = '\0';
	if(pPlayer->m_Stats.m_ShotsFired)
		str_format(aUntrackedOrAccuracy, sizeof(aUntrackedOrAccuracy), " (%.2f%% hit accuracy)", pPlayer->m_Stats.HitAccuracy());
	if(!pSelf->m_pController->IsStatTrack())
		str_format(aUntrackedOrAccuracy, sizeof(aUntrackedOrAccuracy), " (%d untracked)", pPlayer->m_Kills);
	str_format(aBuf, sizeof(aBuf), "~ Kills: %d%s", pPlayer->m_Stats.m_Kills, aUntrackedOrAccuracy);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);

	char aUntracked[512];
	aUntracked[0] = '\0';
	if(!pSelf->m_pController->IsStatTrack())
		str_format(aUntracked, sizeof(aUntracked), " (%d untracked)", pPlayer->m_Deaths);
	str_format(aBuf, sizeof(aBuf), "~ Deaths: %d%s", pPlayer->m_Stats.m_Deaths, aUntracked);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);

	aUntracked[0] = '\0';
	if(!pSelf->m_pController->IsStatTrack())
		str_format(aUntracked, sizeof(aUntracked), " (%d untracked)", pPlayer->m_UntrackedSpree);
	str_format(aBuf, sizeof(aBuf), "~ Current killing spree: %d%s", pPlayer->Spree(), aUntracked);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Highest killing spree: %d", pPlayer->m_Stats.m_BestSpree);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);

	pSelf->m_pController->OnShowRoundStats(&pPlayer->m_Stats, pRequestingPlayer, pName);
}

void CGameContext::ConStatsAllTime(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowStats(pResult->m_ClientId, pName, pSelf->m_pController->StatsTable(), EInstaSqlRequestType::CHAT_CMD_STATSALL);
}

void CGameContext::ConMultis(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(!pSelf->m_pController->IsFngGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command only available in fng gametypes.");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowStats(pResult->m_ClientId, pName, pSelf->m_pController->StatsTable(), EInstaSqlRequestType::CHAT_CMD_MULTIS);
}

void CGameContext::ConSteals(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(!pSelf->m_pController->IsFngGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command only available in fng gametypes.");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowStats(pResult->m_ClientId, pName, pSelf->m_pController->StatsTable(), EInstaSqlRequestType::CHAT_CMD_STEALS);
}

void CGameContext::ConRoundTop(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->SendRoundTopMessage(pResult->m_ClientId);
}

void CGameContext::ConScore(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(pResult->NumArguments() == 0)
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "Your current display score type is %s.", display_score_to_str(pPlayer->m_DisplayScore));
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		pSelf->SendChatTarget(pResult->m_ClientId, "You can change it to any of these: " DISPLAY_SCORE_VALUES);
		return;
	}

	if(str_to_display_score(pResult->GetString(0), &pPlayer->m_DisplayScore))
		pSelf->SendChatTarget(pResult->m_ClientId, "Updated display score.");
	else
		pSelf->SendChatTarget(pResult->m_ClientId, "Invalid score name pick one of those: " DISPLAY_SCORE_VALUES);
}

void CGameContext::ConRankKills(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowRank(pResult->m_ClientId, pName, "Kills", "kills", pSelf->m_pController->StatsTable(), "DESC");
}

void CGameContext::ConInstaRankPoints(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConPoints(pResult, pUserData);
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowRank(pResult->m_ClientId, pName, "Points", "points", pSelf->m_pController->StatsTable(), "DESC");
}

void CGameContext::ConTopKills(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId);
	int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1;
	pSelf->m_pController->m_pSqlStats->ShowTop(pResult->m_ClientId, pName, "Kills", "kills", pSelf->m_pController->StatsTable(), "DESC", Offset);
}

void CGameContext::ConRankFastcaps(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	if(!(pSelf->m_pController->GameFlags() & GAMEFLAG_FLAGS))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "this gamemode has no flags");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowFastcapRank(
		pResult->m_ClientId,
		pName,
		pSelf->Server()->GetMapName(),
		pSelf->m_pController->m_pGameType,
		pSelf->m_pController->IsGrenadeGameType(),
		false); // show all times stat track or not
}

void CGameContext::ConTopFastcaps(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	if(!(pSelf->m_pController->GameFlags() & GAMEFLAG_FLAGS))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "this gamemode has no flags");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1;
	pSelf->m_pController->m_pSqlStats->ShowFastcapTop(
		pResult->m_ClientId,
		pName,
		pSelf->Server()->GetMapName(),
		pSelf->m_pController->m_pGameType,
		pSelf->m_pController->IsGrenadeGameType(),
		false, // show all times stat track or not
		Offset);
}

void CGameContext::ConTopNumCaps(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId);
	int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1;
	pSelf->m_pController->m_pSqlStats->ShowTop(pResult->m_ClientId, pName, "Flag captures", "flag_captures", pSelf->m_pController->StatsTable(), "DESC", Offset);
}

void CGameContext::ConRankFlagCaptures(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	if(!(pSelf->m_pController->GameFlags() & GAMEFLAG_FLAGS))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "this gamemode has no flags");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->m_pSqlStats->ShowRank(pResult->m_ClientId, pName, "Flag captures", "flag_captures", pSelf->m_pController->StatsTable(), "DESC");
}

void CGameContext::ConTopSpikeColors(IConsole::IResult *pResult, void *pUserData)
{
	const auto *pSelf = static_cast<CGameContext *>(pUserData);
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(!pSelf->m_pController->IsFngGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command only available in fng gametypes.");
		return;
	}

	const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId);
	const char *pSpikeColor = pResult->GetString(0);
	const int Offset = pResult->NumArguments() > 1 ? pResult->GetInteger(1) : 1;
	const char *apSpikeColors[] = {
		"gold",
		"green",
		"purple"};

	for(const char *pColor : apSpikeColors)
	{
		if(str_comp_nocase(pSpikeColor, pColor) == 0)
		{
			char aDisplayName[64];
			str_format(aDisplayName, sizeof(aDisplayName), "%s spikes", pColor);

			char aDbColumn[64];
			str_format(aDbColumn, sizeof(aDbColumn), "%s_spikes", pColor);

			pSelf->m_pController->m_pSqlStats->ShowTop(
				pResult->m_ClientId, pName,
				aDisplayName,
				aDbColumn,
				pSelf->m_pController->StatsTable(),
				"DESC",
				Offset);
			return;
		}
	}

	pSelf->SendChatTarget(pResult->m_ClientId, "~~~ Usage: /top5spikes <color> - Available colors:");
	for(const char *pColor : apSpikeColors)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "~ %s", pColor);
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
	}
}

#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	void CGameContext::ConInstaRank##name(IConsole::IResult *pResult, void *pUserData) \
	{ \
		CGameContext *pSelf = (CGameContext *)pUserData; \
		if(!CheckClientId(pResult->m_ClientId)) \
			return; \
		if(!pSelf->m_pController) \
			return; \
		if(pSelf->m_pController->IsDDRaceGameType()) \
		{ \
			pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes."); \
			return; \
		} \
\
		const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId); \
		pSelf->m_pController->m_pSqlStats->ShowRank(pResult->m_ClientId, pName, display_name, #sql_name, pSelf->m_pController->StatsTable(), order_by); \
	}
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	void CGameContext::ConInstaTop##name(IConsole::IResult *pResult, void *pUserData) \
	{ \
		CGameContext *pSelf = (CGameContext *)pUserData; \
		if(!CheckClientId(pResult->m_ClientId)) \
			return; \
		if(!pSelf->m_pController) \
			return; \
		if(pSelf->m_pController->IsDDRaceGameType()) \
		{ \
			pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes."); \
			return; \
		} \
\
		const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId); \
		int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1; \
		pSelf->m_pController->m_pSqlStats->ShowTop(pResult->m_ClientId, pName, display_name, #sql_name, pSelf->m_pController->StatsTable(), order_by, Offset); \
	}
#include <game/server/instagib/sql_colums_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN
