#include <algorithm>
#include <base/log.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/server/instagib/sql_accounts.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/player.h>

#include "base_pvp.h"

void CGameControllerPvp::OnLogin(const CAccount *pAccount, class CPlayer *pPlayer)
{
	if(!g_Config.m_SvAccounts)
	{
		GameServer()->SendChatTarget(pPlayer->GetCid(), "Login failed. Accounts were turned off by an admin");
		return;
	}

	GameServer()->SendChatTarget(pPlayer->GetCid(), "Successfully logged in");

	pPlayer->m_Account = *pAccount;
	pPlayer->m_Account.m_IsLoggedIn = true;
}

void CGameControllerPvp::OnRegister(class CPlayer *pPlayer)
{
	GameServer()->SendChatTarget(pPlayer->GetCid(), "Successfully registered an account, you can login now");
}

void CGameControllerPvp::LogoutAccount(class CPlayer *pPlayer, const char *pSuccessMessage)
{
	if(!pPlayer->m_Account.IsLoggedIn())
		return;

	m_pSqlStats->SaveAndLogoutAccount(pPlayer, pSuccessMessage);
}

void CGameControllerPvp::OnLogout(class CPlayer *pPlayer, const char *pMessage)
{
	pPlayer->m_Account.m_IsLoggedIn = false;
	SendChatTarget(pPlayer->GetCid(), pMessage);
}

void CGameControllerPvp::LogoutAllAccounts()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		LogoutAccount(pPlayer, "Logged out of account");
	}
}

void CGameControllerPvp::OnShutdown()
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

void CGameControllerPvp::RequestChangePassword(class CPlayer *pPlayer, const char *pOldPassword, const char *pNewPassword)
{
	dbg_assert(pPlayer->m_Account.IsLoggedIn(), "player without active account tried to change password");
	m_pSqlStats->Account(pPlayer->GetCid(), pPlayer->m_Account.m_aUsername, pOldPassword, pNewPassword, EAccountPlayerRequestType::CHAT_CMD_CHANGE_PASSWORD);
}

void CGameControllerPvp::OnChangePassword(class CPlayer *pPlayer)
{
	SendChatTarget(pPlayer->GetCid(), "Password updated successfully");
}

void CGameControllerPvp::OnFailedAccountLogin(class CPlayer *pPlayer, const char *pErrorMsg)
{
	SendChatTarget(pPlayer->GetCid(), pErrorMsg);
	CIpRatelimit::TrackWrongLogin(m_vIpRatelimits, Server()->ClientAddr(pPlayer->GetCid()), Server()->Tick());
}

bool CGameControllerPvp::IsAccountRatelimited(int ClientId, char *pReason, int ReasonSize)
{
	if(pReason)
		pReason[0] = '\0';

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
CPlayer *CGameControllerPvp::GetPlayerByAccountUsername(const char *pUsername)
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

void CGameControllerPvp::AccountList(const char *pSearch)
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

bool CGameControllerPvp::IsAccountRconCmdRatelimited(int ClientId, char *pReason, int ReasonSize)
{
	if(pReason)
		pReason[0] = '\0';

	CPlayer *pPlayer = GetPlayerOrNullptr(ClientId);
	if(!pPlayer)
	{
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

			str_copy(pReason, "already pending rcon cmd", ReasonSize);
			return true;
		});
	if(PendingRconCmd)
		return true;

	return false;
}

void CGameControllerPvp::RconForceSetPassword(class CPlayer *pPlayer, const char *pUsername, const char *pPassword)
{
	m_pSqlStats->AccountRconCmd(pPlayer->GetCid(), pUsername, pPassword, EAccountRconPlayerRequestType::ACC_SET_PASSWORD);
}

void CGameControllerPvp::RconForceLogout(class CPlayer *pPlayer, const char *pUsername)
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

	m_pSqlStats->AccountRconCmd(pPlayer->GetCid(), pUsername, "", EAccountRconPlayerRequestType::ACC_LOGOUT);
}

void CGameControllerPvp::RconLockAccount(class CPlayer *pPlayer, const char *pUsername)
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
	m_pSqlStats->AccountRconCmd(pPlayer->GetCid(), pUsername, "", EAccountRconPlayerRequestType::ACC_LOCK);
}

void CGameControllerPvp::RconUnlockAccount(class CPlayer *pPlayer, const char *pUsername)
{
	m_pSqlStats->AccountRconCmd(pPlayer->GetCid(), pUsername, "", EAccountRconPlayerRequestType::ACC_UNLOCK);
}

void CGameControllerPvp::RconAccountInfo(class CPlayer *pPlayer, const char *pUsername)
{
	m_pSqlStats->AccountRconCmd(pPlayer->GetCid(), pUsername, "", EAccountRconPlayerRequestType::ACC_INFO);
}

void CGameControllerPvp::OnAccountInfo(int AdminUniqueClientId, const char *pUsername, CAccount *pAccount)
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

void CGameControllerPvp::ProcessAccountRconCmdResult(CAccountRconCmdResult &Result)
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
