#include <base/str.h>
#include <base/time.h>

#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <game/server/score.h>

#include <insta/server/enums.h>

// "/info"
bool CGameContext::BlockAccountOperation(CGameContext *pSelf, int ClientId, const char *pOperation)
{
	if(!CheckClientId(ClientId))
		return true;

	if(!pSelf->m_pController)
	{
		pSelf->SendChatTarget(ClientId, "Something went wrong with this account request");
		return true;
	}

	if(!g_Config.m_SvAccounts)
	{
		pSelf->SendChatTarget(ClientId, "Accounts are turned off");
		return true;
	}

	char aReason[512];
	if(pSelf->m_pController->IsAccountRatelimited(ClientId, aReason, sizeof(aReason)))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%s failed because of: %s", pOperation, aReason);
		pSelf->SendChatTarget(ClientId, aBuf);
		return true;
	}
	return false;
}

void CGameContext::ConInstaInfo(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pSelf->m_pController)
		pSelf->m_pController->OnInfoChatCmd(pResult, pUserData);
}

// "/credits"
void CGameContext::ConInstaModeCredits(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(pSelf->m_pController)
		pSelf->m_pController->OnCreditsChatCmd(pResult, pUserData);
}

// "/credits_insta"
void CGameContext::ConInstaCredits(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->PrintInstaCredits();
}

void CGameContext::ConInstaTeam(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	// ddnet-insta
	if(pSelf->m_pController->OnTeamChatCmd(pResult))
		return;

	// ddnet
	ConTeam(pResult, pUserData);
}

void CGameContext::ConInstaLock(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	if(!g_Config.m_SvAllowDDRaceTeamChange)
	{
		log_info("chatresp", "The /lock chat command is currently disabled.");
		return;
	}

	// ddnet
	ConLock(pResult, pUserData);
}

void CGameContext::ConInstaUnlock(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	if(!g_Config.m_SvAllowDDRaceTeamChange)
	{
		log_info("chatresp", "The /unlock chat command is currently disabled.");
		return;
	}

	// ddnet
	ConUnlock(pResult, pUserData);
}

void CGameContext::ConInstaInvite(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	if(!g_Config.m_SvAllowDDRaceTeamChange)
	{
		log_info("chatresp", "The /invite chat command is currently disabled.");
		return;
	}

	// ddnet
	ConInvite(pResult, pUserData);
}

void CGameContext::ConInstaJoin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	if(!g_Config.m_SvAllowDDRaceTeamChange)
	{
		log_info("chatresp", "The /join chat command is currently disabled.");
		return;
	}

	// ddnet
	ConJoin(pResult, pUserData);
}

void CGameContext::ConInstaTeam0Mode(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	if(!g_Config.m_SvAllowDDRaceTeamChange)
	{
		log_info("chatresp", "The /team0mode chat command is currently disabled.");
		return;
	}

	// ddnet
	ConTeam0Mode(pResult, pUserData);
}

void CGameContext::ConInstaTogglePause(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	pSelf->m_pController->OnPauseChatCmd(pResult, pUserData);
}

void CGameContext::ConInstaToggleSpec(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;

	pSelf->m_pController->OnSpecChatCmd(pResult, pUserData);
}

void CGameContext::ConInstaTogglePauseVoted(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConTogglePauseVoted(pResult, pUserData);
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", "Only spectators can use this command.");
		return;
	}

	ConTogglePauseVoted(pResult, pUserData);
}

void CGameContext::ConInstaToggleSpecVoted(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConToggleSpecVoted(pResult, pUserData);
		return;
	}

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", "Only spectators can use this command.");
		return;
	}

	ConToggleSpecVoted(pResult, pUserData);
}

void CGameContext::ConInstaKill(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!pSelf->m_pController)
		return;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	pSelf->m_pController->OnKillChatCmd(pResult, pUserData);
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

	if(pSelf->m_pController->IsDDRaceGameType())
	{
		ConSwap(pResult, pUserData);
		return;
	}

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

	// ddrace finish times are less interesting in block than kill rankings
	if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType())
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

	// ddrace finish times are less interesting in block than kill rankings
	if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType())
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

	if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType())
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
		str_format(aUntrackedOrAccuracy, sizeof(aUntrackedOrAccuracy), " (%d untracked)", pPlayer->m_RoundStats.m_Kills);
	str_format(aBuf, sizeof(aBuf), "~ Kills: %d%s", pPlayer->m_Stats.m_Kills, aUntrackedOrAccuracy);
	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", aBuf);

	char aUntracked[512];
	aUntracked[0] = '\0';
	if(!pSelf->m_pController->IsStatTrack())
		str_format(aUntracked, sizeof(aUntracked), " (%d untracked)", pPlayer->m_RoundStats.m_Deaths);
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

	if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->Db()->Stats()->ShowStats(pResult->m_ClientId, pName, pSelf->m_pController->StatsTable(), EInstaSqlRequestType::CHAT_CMD_STATSALL);
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
	pSelf->m_pController->Db()->Stats()->ShowStats(pResult->m_ClientId, pName, pSelf->m_pController->StatsTable(), EInstaSqlRequestType::CHAT_CMD_MULTIS);
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
	pSelf->m_pController->Db()->Stats()->ShowStats(pResult->m_ClientId, pName, pSelf->m_pController->StatsTable(), EInstaSqlRequestType::CHAT_CMD_STEALS);
}

void CGameContext::ConRoundTop(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	pSelf->m_pController->SendRoundTopMessage(pResult->m_ClientId);
}

void CGameContext::ConRegister(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountOperation(pSelf, pResult->m_ClientId, "Register"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	const char *pUsername = pResult->GetString(0);
	const char *pPassword = pResult->GetString(1);
	const char *pPasswordRepeat = pResult->GetString(2);

	if(str_comp(pPassword, pPasswordRepeat))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "Passwords do not match");
		return;
	}
	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, pPassword, aBuf, sizeof(aBuf)))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}
	if(g_Config.m_SvPointsNeededToRegister)
	{
		int Score = pPlayer->m_Score;
		bool KillsNeeded = pSelf->m_pController->IsZcatchGameType();
		if(KillsNeeded)
			Score = pPlayer->m_RoundStats.m_Kills;
		int Missing = g_Config.m_SvPointsNeededToRegister - Score;
		if(Missing > 0)
		{
			str_format(aBuf, sizeof(aBuf), "You you need %d more %s to create an account", Missing, KillsNeeded ? "kills" : "points");
			pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
			return;
		}
	}

	int SecondsConnected = (time_get() - pPlayer->m_FirstJoinTime) / time_freq();
	int SecondsUntilAllowed = std::max(0, g_Config.m_SvJoinRegisterDelay - SecondsConnected);
	if(SecondsUntilAllowed)
	{
		str_format(aBuf, sizeof(aBuf), "Please wait %d more seconds before registering an account.", SecondsUntilAllowed);
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}

	// this is to avoid logged in players
	// getting locked by antispam/brutforce protection
	if(pPlayer->m_Account.IsLoggedIn())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "You are already logged in. Logout first to register a new account.");
		return;
	}

	pSelf->m_pController->Db()->Accounts()->ChatCmd(
		pResult->m_ClientId,
		pUsername,
		pSelf->Server()->ClientName(pResult->m_ClientId),
		pPassword,
		pPassword,
		EAccountChatCmd::CHAT_CMD_REGISTER);
}

void CGameContext::ConLogin(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountOperation(pSelf, pResult->m_ClientId, "Login"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(pPlayer->m_Account.IsLoggedIn())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "You are already logged in");
		return;
	}

	const char *pUsername = pResult->GetString(0);
	const char *pPassword = pResult->GetString(1);

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pUsername, pPassword, aBuf, sizeof(aBuf)))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}

	pSelf->m_pController->Db()->Accounts()->ChatCmd(
		pResult->m_ClientId,
		pUsername,
		pSelf->Server()->ClientName(pResult->m_ClientId),
		pPassword,
		pPassword,
		EAccountChatCmd::CHAT_CMD_LOGIN);
}

void CGameContext::ConLogoutAccount(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountOperation(pSelf, pResult->m_ClientId, "Logout"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pPlayer->m_Account.IsLoggedIn())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "You are not logged in");
		return;
	}

	pSelf->m_pController->LogoutAccount(pPlayer, "Successfully logged out of your account");
}

void CGameContext::ConChangePassword(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountOperation(pSelf, pResult->m_ClientId, "Change password"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pPlayer->m_Account.IsLoggedIn())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "You are not logged in");
		return;
	}

	// old password could be checked against a cached password
	// but just to be sure we check in the db thread against the latest db password
	// then we also do not have to hold passwords in ram which seems like a security win

	const char *pOldPassword = pResult->GetString(0);
	const char *pNewPassword = pResult->GetString(1);
	const char *pNewPasswordRepeat = pResult->GetString(2);

	if(str_comp(pNewPassword, pNewPasswordRepeat))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "New passwords do not match");
		return;
	}

	char aBuf[512];
	if(!IsValidUsernameAndPassword(pPlayer->m_Account.m_aUsername, pNewPassword, aBuf, sizeof(aBuf)))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}
	if(!IsValidUsernameAndPassword(pPlayer->m_Account.m_aUsername, pOldPassword, aBuf, sizeof(aBuf)))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}

	pSelf->m_pController->RequestChangePassword(pPlayer, pOldPassword, pNewPasswordRepeat);
}

void CGameContext::ConDisplayName(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountOperation(pSelf, pResult->m_ClientId, "Display name"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pPlayer->m_Account.IsLoggedIn())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "You are not logged in");
		return;
	}

	if(g_Config.m_SvClaimableNames < 2)
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is currently deactivated");
		return;
	}

	char aBuf[512];
	const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId);

	bool InvalidName = false;

	// There might be some edge case where modded clients
	// manage to request an empty name
	// standard clients fall back to "nameless tee"
	// and the server falls back to "(1)"
	// but just to be sure to avoid some annoying bug
	if(pName[0] == '\0')
		InvalidName = true;

	// There are a few magic names used by the teeworlds engine
	// for connecting and disconnected players
	// I could imagine some nasty edge case bugs if these get claimed
	if(!str_comp_nocase(pName, "(invalid)") || !str_comp_nocase(pName, "(connecting)"))
		InvalidName = true;

	// "(..)" is ddnet-insta's magic prefix for unverified names
	// to avoid thinking about all the edge cases for when someone claims
	// the name "(..) (..)" and someone claims the name "(..)"
	// and someone joins with the name "(..)" but is not verified
	// and the server prefixes it to "(..) (..)" which results in a collision
	// with a claimed name
	if(str_startswith(pName, "(..)"))
		InvalidName = true;

	if(InvalidName)
	{
		str_format(aBuf, sizeof(aBuf), "The name '%s' can not be claimed", pName);
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}

	if(auto Iter = pSelf->m_UnclaimableNames.find(pName); Iter != pSelf->m_UnclaimableNames.end())
	{
		str_format(aBuf, sizeof(aBuf), "The name '%s' can not be claimed (please contact server staff)", pName);
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}

	if(!str_comp(pPlayer->m_Account.m_aDisplayName, pName))
	{
		str_format(aBuf, sizeof(aBuf), "You already claimed the name '%s' nobody else can use it", pName);
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
		return;
	}

	pSelf->m_pController->ChatCmdDisplayName(pPlayer);
}

void CGameContext::ConLockName(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountOperation(pSelf, pResult->m_ClientId, "Lock name"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pPlayer->m_Account.IsLoggedIn())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "You are not logged in");
		return;
	}

	if(g_Config.m_SvClaimableNames < 2)
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is currently deactivated");
		return;
	}

	if(pPlayer->m_Account.m_aDisplayName[0] == '\0')
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "You first need to set a name using the command /displayname");
		return;
	}

	if(str_comp(pPlayer->m_Account.m_aDisplayName, pSelf->Server()->ClientName(pResult->m_ClientId)))
	{
		// Should this be an error instead of a warning?
		// This is only about ux not about security or robustness.
		// This can be done either way by changing the name while the db operation is pending.
		//
		// The question is do we want to fail loud with one clear error and tell the user what to do.
		// Or do it anyways and just print a warning mixed with a success message.
		//
		// I think the warning is better even if there is a bit much text.
		// Because it supports users locking their name as soon as they realize someone stole it.
		// So they can not even use the name right now.
		char aBuf[1024];
		str_format(
			aBuf,
			sizeof(aBuf),
			"Warning the locked name will be '%s' and not '%s' use the /displayname to change your name",
			pPlayer->m_Account.m_aDisplayName,
			pSelf->Server()->ClientName(pResult->m_ClientId));
		pSelf->SendChatTarget(pResult->m_ClientId, aBuf);
	}

	pSelf->m_pController->ChatCmdLockName(pPlayer);
}

void CGameContext::ConSlowAccountOperation(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(BlockAccountOperation(pSelf, pResult->m_ClientId, "Slow account operation"))
		return;

	CPlayer *pPlayer = pSelf->m_apPlayers[pResult->m_ClientId];
	if(!pPlayer)
		return;

	if(!pSelf->Server()->GetAuthedState(pResult->m_ClientId))
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "Missing permissions.");
		return;
	}

	if(!g_Config.m_SvTestingCommands)
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "Test commands are turned off");
		return;
	}

	pSelf->m_pController->Db()->Accounts()->ChatCmdSlowOperation(pResult->m_ClientId);
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

	if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->Db()->Stats()->ShowRank(pResult->m_ClientId, pName, "Kills", "kills", pSelf->m_pController->StatsTable(), "DESC");
}

void CGameContext::ConInstaRankPoints(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType())
	{
		ConPoints(pResult, pUserData);
		return;
	}

	const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId);
	pSelf->m_pController->Db()->Stats()->ShowRank(pResult->m_ClientId, pName, "Points", "points", pSelf->m_pController->StatsTable(), "DESC");
}

void CGameContext::ConTopKills(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext *pSelf = (CGameContext *)pUserData;
	if(!CheckClientId(pResult->m_ClientId))
		return;

	if(!pSelf->m_pController)
		return;

	if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes.");
		return;
	}

	const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId);
	int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1;
	pSelf->m_pController->Db()->Stats()->ShowTop(pResult->m_ClientId, pName, "Kills", "kills", pSelf->m_pController->StatsTable(), "DESC", Offset);
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
	pSelf->m_pController->Db()->Stats()->ShowFastcapRank(
		pResult->m_ClientId,
		pName,
		pSelf->Map()->BaseName(),
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
	pSelf->m_pController->Db()->Stats()->ShowFastcapTop(
		pResult->m_ClientId,
		pName,
		pSelf->Map()->BaseName(),
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
	pSelf->m_pController->Db()->Stats()->ShowTop(pResult->m_ClientId, pName, "Flag captures", "flag_captures", pSelf->m_pController->StatsTable(), "DESC", Offset);
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
	pSelf->m_pController->Db()->Stats()->ShowRank(pResult->m_ClientId, pName, "Flag captures", "flag_captures", pSelf->m_pController->StatsTable(), "DESC");
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

			pSelf->m_pController->Db()->Stats()->ShowTop(
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

void CGameContext::ConSetSpawn(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CGameContext *>(pUserData);
	if(!pSelf->m_pController->IsTrainFngGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in this mode.");
		return;
	}

	CCharacter *pChr = pSelf->GetPracticeCharacter(pResult);
	if(!pChr)
		return;

	CPlayer *pPlayer = pChr->GetPlayer();
	if(!pPlayer)
		return;

	if(!pPlayer->m_pTrainSave)
		pPlayer->m_pTrainSave = new CSaveTee();

	pPlayer->m_pTrainSave->Save(pChr);
	pSelf->SendChatTarget(pResult->m_ClientId, "Spawn position updated");
}

void CGameContext::ConSpawnReset(IConsole::IResult *pResult, void *pUserData)
{
	auto *pSelf = static_cast<CGameContext *>(pUserData);
	if(!pSelf->m_pController->IsTrainFngGameType())
	{
		pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in this mode.");
		return;
	}

	CCharacter *pChr = pSelf->GetPracticeCharacter(pResult);
	if(!pChr)
		return;

	CPlayer *pPlayer = pChr->GetPlayer();
	if(!pPlayer || !pPlayer->m_pTrainSave)
		return;

	delete pPlayer->m_pTrainSave;
	pPlayer->m_pTrainSave = nullptr;
	pSelf->SendChatTarget(pResult->m_ClientId, "Spawn position reset");
}

// NOLINTBEGIN(misc-definitions-in-headers)
#define MACRO_ADD_COLUMN(name, sql_name, sql_type, bind_type, default, merge_method) ;
#define MACRO_RANK_COLUMN(name, sql_name, display_name, order_by) \
	void CGameContext::ConInstaRank##name(IConsole::IResult *pResult, void *pUserData) \
	{ \
		CGameContext *pSelf = (CGameContext *)pUserData; \
		if(!CheckClientId(pResult->m_ClientId)) \
			return; \
		if(!pSelf->m_pController) \
			return; \
		if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType()) \
		{ \
			pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes."); \
			return; \
		} \
\
		const char *pName = pResult->NumArguments() ? pResult->GetString(0) : pSelf->Server()->ClientName(pResult->m_ClientId); \
		pSelf->m_pController->Db()->Stats()->ShowRank(pResult->m_ClientId, pName, display_name, #sql_name, pSelf->m_pController->StatsTable(), order_by); \
	}
#define MACRO_TOP_COLUMN(name, sql_name, display_name, order_by) \
	void CGameContext::ConInstaTop##name(IConsole::IResult *pResult, void *pUserData) \
	{ \
		CGameContext *pSelf = (CGameContext *)pUserData; \
		if(!CheckClientId(pResult->m_ClientId)) \
			return; \
		if(!pSelf->m_pController) \
			return; \
		if(pSelf->m_pController->IsDDRaceGameType() && !pSelf->m_pController->IsBlockGameType()) \
		{ \
			pSelf->SendChatTarget(pResult->m_ClientId, "This command is not available in ddrace gametypes."); \
			return; \
		} \
\
		const char *pName = pSelf->Server()->ClientName(pResult->m_ClientId); \
		int Offset = pResult->NumArguments() ? pResult->GetInteger(0) : 1; \
		pSelf->m_pController->Db()->Stats()->ShowTop(pResult->m_ClientId, pName, display_name, #sql_name, pSelf->m_pController->StatsTable(), order_by, Offset); \
	}
#include <insta/server/sql_columns_all.h>
#undef MACRO_ADD_COLUMN
#undef MACRO_RANK_COLUMN
#undef MACRO_TOP_COLUMN
// NOLINTEND(misc-definitions-in-headers)
