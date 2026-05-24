#include "insta_core.h"

#include <base/log.h>
#include <base/time.h>

#include <engine/console.h>
#include <engine/shared/config.h>

#include <game/server/player.h>

#include <insta/server/sql_accounts.h>
#include <insta/server/sql_stats.h>

#include <algorithm>

void CGameControllerInstaCore::EnableAccTable(EExtraAccTable Table)
{
	if(!m_pExtraAccountTableController)
	{
		m_pExtraAccountTableController = new CExtraAccountTableController();
	}

	// TODO: error on duplicated entries
	m_pExtraAccountTableController->m_vTables.emplace_back(Table);
}

void CGameControllerInstaCore::OnLogin(const CAccount *pAccount, class CPlayer *pPlayer)
{
	if(!g_Config.m_SvAccounts)
	{
		GameServer()->SendChatTarget(pPlayer->GetCid(), "Login failed. Accounts were turned off by an admin");
		return;
	}

	GameServer()->SendChatTarget(pPlayer->GetCid(), "Successfully logged in");

	pPlayer->m_Account = *pAccount;
	pPlayer->m_Account.m_IsLoggedIn = true;

	bool CouldNotUseName = !pPlayer->m_DisplayName.CanUseName();
	pPlayer->m_DisplayName.SetAccountUsername(pAccount->Username());

	if(pPlayer->m_DisplayName.CanUseName() && CouldNotUseName)
	{
		GameServer()->SendChatTarget(pPlayer->GetCid(), "You can use this name because you logged in");
		GameServer()->ChangeName(
			pPlayer->GetCid(),
			pPlayer->m_DisplayName.DisplayName(),
			/*
			 * Do not show a "changed the name" chat message
			 * if the user did not request a name change
			 * this is just changing the name from a pending
			 * name to a confirmed name
			 *
			 */
			pPlayer->m_DisplayName.NumChanges() == 1);
	}
}

void CGameControllerInstaCore::OnRegister(class CPlayer *pPlayer)
{
	GameServer()->SendChatTarget(pPlayer->GetCid(), "Successfully registered an account, you can login now");
}

void CGameControllerInstaCore::LogoutAccount(class CPlayer *pPlayer, const char *pSuccessMessage)
{
	if(!pPlayer->m_Account.IsLoggedIn())
		return;

	m_pSqlStats->SaveAndLogoutAccount(pPlayer, pSuccessMessage);
	pPlayer->m_DisplayName.SetAccountUsername("");
	if(!pPlayer->m_DisplayName.CanUseName())
	{
		GameServer()->SendChatTarget(pPlayer->GetCid(), "You can no longer use this name because you logged out");
		GameServer()->ChangeName(pPlayer->GetCid(), pPlayer->m_DisplayName.DisplayName(), false);
	}
}

void CGameControllerInstaCore::OnLogout(class CPlayer *pPlayer, const char *pMessage)
{
	pPlayer->m_Account.m_IsLoggedIn = false;
	SendChatTarget(pPlayer->GetCid(), pMessage);
}

void CGameControllerInstaCore::LogoutAllAccounts()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		LogoutAccount(pPlayer, "Logged out of account");
	}
}

void CGameControllerInstaCore::OnShutdown()
{
	if(!g_Config.m_SvAccounts)
		return;

	// to improve shutdown performance
	// we do not start one sql request for every logged in player
	// but just log players out on the application level using ``OnLogout()``
	// and then task the db once to logout all accounts on the current server

	log_info("ddnet-insta", "logging out all accounts ...");

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		OnLogout(pPlayer, "Logged out of account because of server shutdown");
	}

	m_pSqlStats->LogoutAllAccountsOnCurrentServer();
}

void CGameControllerInstaCore::RequestChangePassword(class CPlayer *pPlayer, const char *pOldPassword, const char *pNewPassword)
{
	dbg_assert(pPlayer->m_Account.IsLoggedIn(), "player without active account tried to change password");
	m_pSqlStats->Account(pPlayer->GetCid(), pPlayer->m_Account.m_aUsername, Server()->ClientName(pPlayer->GetCid()), pOldPassword, pNewPassword, EAccountPlayerRequestType::CHAT_CMD_CHANGE_PASSWORD);
}

void CGameControllerInstaCore::OnChangePassword(class CPlayer *pPlayer)
{
	SendChatTarget(pPlayer->GetCid(), "Password updated successfully");
}

void CGameControllerInstaCore::OnFailedAccountLogin(class CPlayer *pPlayer, const char *pErrorMsg)
{
	SendChatTarget(pPlayer->GetCid(), pErrorMsg);
	CIpRatelimit::TrackWrongLogin(m_vIpRatelimits, Server()->ClientAddr(pPlayer->GetCid()), Server()->Tick());
}

void CGameControllerInstaCore::RequestClaimName(class CPlayer *pPlayer)
{
	m_pSqlStats->Account(pPlayer->GetCid(), pPlayer->m_Account.m_aUsername, Server()->ClientName(pPlayer->GetCid()), "", "", EAccountPlayerRequestType::CHAT_CMD_CLAIM_NAME);
}

void CGameControllerInstaCore::OnNameClaimed(class CPlayer *pPlayer, const char *pDisplayName, const char *pUsername)
{
	// the player can logout or switch accounts
	// while the name claim is pending
	// in that case we drop this event
	if(!pPlayer->m_Account.IsLoggedIn())
		return;
	if(str_comp(pPlayer->m_Account.Username(), pUsername))
		return;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "You claimed the name '%s'. Nobody else can use it now.", pDisplayName);
	GameServer()->SendChatTarget(pPlayer->GetCid(), aBuf);

	str_copy(pPlayer->m_Account.m_aDisplayName, pDisplayName);

	// the player could have performed a name change
	// while the name claim was pending
	if(!str_comp(pPlayer->m_DisplayName.WantedName(), pDisplayName))
		pPlayer->m_DisplayName.SetNameOwner(pUsername);
}

bool CGameControllerInstaCore::IsAccountRatelimited(int ClientId, char *pReason, int ReasonSize)
{
	if(pReason)
		pReason[0] = '\0';

	// econ and fifo have no ratelimits
	if(ClientId == IConsole::CLIENT_ID_UNSPECIFIED)
		return false;

	// rate limit if delay to last query is too short
	if(m_pSqlStats->IsRateLimitedPlayer(ClientId))
	{
		str_copy(pReason, "ratelimited", ReasonSize);
		return true;
	}

	CPlayer *pPlayer = GetPlayerOrNullptr(ClientId);
	if(!pPlayer)
	{
		str_copy(pReason, "invalid player", ReasonSize);
		return false;
	}

	// rate limit if we are still waiting on the previous
	// operation to finish
	if(pPlayer->m_AccountQueryResult != nullptr)
	{
		str_copy(pReason, "already pending operation", ReasonSize);
		return true;
	}

	if(pPlayer->m_AccountLogoutQueryResult != nullptr)
	{
		str_copy(pReason, "pending logout", ReasonSize);
		return true;
	}

	const NETADDR *pAddr = Server()->ClientAddr(pPlayer->GetCid());
	if(CIpRatelimit::IsLoginRatelimited(m_vIpRatelimits, pAddr))
	{
		str_copy(pReason, "your ip is ratelimited", ReasonSize);
		return true;
	}

	return false;
}

// TODO: should there be a CAccountsController instead?
//       Accounts()->GetPlayerByUsername() would read nicer
CPlayer *CGameControllerInstaCore::GetPlayerByAccountUsername(const char *pUsername)
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(!pPlayer->m_Account.IsLoggedIn())
			continue;
		if(str_comp(pPlayer->m_Account.Username(), pUsername))
			continue;

		return pPlayer;
	}
	return nullptr;
}

// rcon commands

static bool SearchHit(CPlayer *pPlayer, const char *pName, const char *pSearch)
{
	if(pSearch[0] == '\0')
		return true;
	if(str_find_nocase(pName, pSearch))
		return true;
	if(pPlayer->m_Account.IsLoggedIn())
	{
		if(str_find_nocase(pPlayer->m_Account.m_aUsername, pSearch))
			return true;
	}
	// allow searching for client id if its a full match
	char aIdStr[8];
	str_format(aIdStr, sizeof(aIdStr), "%d", pPlayer->GetCid());
	if(!str_comp(aIdStr, pSearch))
		return true;
	return false;
}

void CGameControllerInstaCore::RconAccountList(const char *pSearch)
{
	// list players without account first
	// because in a scrolling console the most relevant
	// things should be at the bottom
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->m_Account.IsLoggedIn())
			continue;
		const char *pName = Server()->ClientName(pPlayer->GetCid());
		if(!SearchHit(pPlayer, pName, pSearch))
			continue;

		log_info("accounts", "cid=%d not logged in name='%s'", pPlayer->GetCid(), pName);
	}

	// list only logged in last
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(!pPlayer->m_Account.IsLoggedIn())
			continue;
		const char *pName = Server()->ClientName(pPlayer->GetCid());
		if(!SearchHit(pPlayer, pName, pSearch))
			continue;

		log_info("accounts", "cid=%d account=%s name='%s'", pPlayer->GetCid(), pPlayer->m_Account.m_aUsername, pName);
	}
}

bool CGameControllerInstaCore::IsAccountRconCmdRatelimited(int ClientId, char *pReason, int ReasonSize)
{
	if(pReason)
		pReason[0] = '\0';

	CPlayer *pPlayer = GetPlayerOrNullptr(ClientId);
	if(!pPlayer)
	{
		if(pReason)
			str_copy(pReason, "invalid player", ReasonSize);
		return false;
	}

	bool PendingRconCmd = std::any_of(
		GameServer()->m_vAccountRconCmdQueryResults.begin(),
		GameServer()->m_vAccountRconCmdQueryResults.end(),
		[&](const std::shared_ptr<CAccountRconCmdResult> &Result) {
			if(!Result)
				return false;
			if(Result->m_Completed)
				return false;
			if(Result->m_UniqueClientId != pPlayer->GetUniqueCid())
				return false;
			return true;
		});
	if(PendingRconCmd)
	{
		if(pReason)
			str_copy(pReason, "already pending rcon cmd", ReasonSize);
		return true;
	}

	return false;
}

void CGameControllerInstaCore::RconForceSetPassword(int ClientId, const char *pUsername, const char *pPassword)
{
	m_pSqlStats->AccountRconCmd(ClientId, pUsername, pPassword, EAccountRconPlayerRequestType::ACC_SET_PASSWORD);
}

void CGameControllerInstaCore::RconForceLogout(int ClientId, const char *pUsername)
{
	// attempt to find the player and save and logout
	// if that does not work run a sql query (this is for accounts that got stuck in a bug)

	CPlayer *pVictim = GetPlayerByAccountUsername(pUsername);
	if(pVictim)
	{
		log_info("ddnet-insta", "logging out account '%s' which is used by online player '%s'", pUsername, Server()->ClientName(pVictim->GetCid()));
		LogoutAccount(pVictim, "You got logged out of your account by an admin");
		return;
	}

	m_pSqlStats->AccountRconCmd(ClientId, pUsername, "", EAccountRconPlayerRequestType::ACC_LOGOUT);
}

void CGameControllerInstaCore::RconLockAccount(int ClientId, const char *pUsername)
{
	CPlayer *pVictim = GetPlayerByAccountUsername(pUsername);
	if(pVictim)
	{
		log_info("ddnet-insta", "locked account '%s' and logged out in game player '%s'", pUsername, Server()->ClientName(pVictim->GetCid()));
		LogoutAccount(pVictim, "Your account got locked by an admin");
	}

	// should be able to lock accounts that are logged in on different servers
	// but it should detect that case and recommend the admin to perform acc_logout manually
	// blocking the acc_lock operation would allow players to avoid getting locked by disconnecting
	// from the server once an admin joins xd
	m_pSqlStats->AccountRconCmd(ClientId, pUsername, "", EAccountRconPlayerRequestType::ACC_LOCK);
}

void CGameControllerInstaCore::RconUnlockAccount(int ClientId, const char *pUsername)
{
	m_pSqlStats->AccountRconCmd(ClientId, pUsername, "", EAccountRconPlayerRequestType::ACC_UNLOCK);
}

void CGameControllerInstaCore::RconAccountInfo(int ClientId, const char *pUsername)
{
	m_pSqlStats->AccountRconCmd(ClientId, pUsername, "", EAccountRconPlayerRequestType::ACC_INFO);
}

void CGameControllerInstaCore::OnAccountInfo(int AdminUniqueClientId, const char *pUsername, CAccount *pAccount)
{
	log_info("account", "account '%s'", pUsername);
	CPlayer *pVictim = GetPlayerByAccountUsername(pUsername);
	if(pVictim)
		log_info("account", " connected on this server id=%d name='%s'", pVictim->GetCid(), Server()->ClientName(pVictim->GetCid()));
	else if(pAccount->m_IsLoggedIn)
		log_info("account", " connected on %s:%d", pAccount->m_aServerIp, pAccount->m_ServerPort);
	else
		log_info("account", " not connected (last seen on %s:%d)", pAccount->m_aServerIp, pAccount->m_ServerPort);

	log_info("account", " locked=%d", pAccount->IsLocked());
	log_info("account", " contact=%s", pAccount->m_aContact);
}

void CGameControllerInstaCore::ProcessAccountRconCmdResult(CAccountRconCmdResult &Result)
{
	if(!Result.m_Success)
	{
		log_error("ddnet-insta", "Account rcon command failed. Check the server logs.");
		return;
	}

	CPlayer *pPlayer = GetPlayerByUniqueId(Result.m_UniqueClientId);

	switch(Result.m_MessageKind)
	{
	case EAccountRconPlayerRequestType::LOG_INFO:
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			log_info("ddnet-insta", "%s", aMessage);
		}
		break;
	case EAccountRconPlayerRequestType::LOG_ERROR:
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			log_error("ddnet-insta", "%s", aMessage);
		}
		break;
	case EAccountRconPlayerRequestType::DIRECT:
		if(!pPlayer)
		{
			// log_warn("ddnet-insta", "lost direct message because player left before sql worker finished");
			return;
		}

		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			GameServer()->SendChatTarget(pPlayer->GetCid(), aMessage);
		}
		break;
	case EAccountRconPlayerRequestType::ALL:
	{
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;

			GameServer()->SendChat(-1, TEAM_ALL, aMessage, -1);
		}
		break;
	}
	case EAccountRconPlayerRequestType::ACC_SET_PASSWORD:
		dbg_assert(false, "Set password used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconPlayerRequestType::ACC_LOGOUT:
		dbg_assert(false, "Logout used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconPlayerRequestType::ACC_LOCK:
		dbg_assert(false, "Lock used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconPlayerRequestType::ACC_UNLOCK:
		dbg_assert(false, "Unlock used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconPlayerRequestType::ACC_INFO:
		OnAccountInfo(Result.m_UniqueClientId, Result.m_aUsername, &Result.m_Account);
		break;
	}
}

void CGameControllerInstaCore::CheckAccountsConfig()
{
	bool AccountsWereOn = g_Config.m_SvAccounts != 0;

	// check activate
	if(g_Config.m_SvAccounts == 0 && GameServer()->m_LastAccountTurnOnAttempt)
	{
		bool PortAndHostSet = g_Config.m_SvPort != 0 && GameServer()->GetHostname(nullptr, 0);
		int SecondsSinceLastAttempt = (time_get() - GameServer()->m_LastAccountTurnOnAttempt) / time_freq();
		if(PortAndHostSet && SecondsSinceLastAttempt < 10)
		{
			log_warn("ddnet-insta", "sv_accounts turned on because sv_port and sv_hostname are now set. Please set sv_accounts after sv_port and sv_hostname in your config.");
			g_Config.m_SvAccounts = 1;
		}
	}

	// check deactivate
	if(g_Config.m_SvAccounts)
	{
		if(g_Config.m_SvPort == 0)
		{
			log_error("ddnet-insta", "sv_accounts can not be turned on if sv_port is 0");
			g_Config.m_SvAccounts = 0;
		}
		if(!GameServer()->GetHostname(nullptr, 0))
		{
			log_error("ddnet-insta", "sv_accounts can not be turned on if sv_hostname is unset");
			g_Config.m_SvAccounts = 0;
		}
	}

	// on deactivate
	if(!g_Config.m_SvAccounts && AccountsWereOn)
	{
		log_info("ddnet-insta", "logging out all players ...");
		LogoutAllAccounts();
	}
}
