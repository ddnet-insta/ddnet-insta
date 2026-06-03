#include "insta_core.h"

#include <base/dbg.h>
#include <base/log.h>
#include <base/str.h>
#include <base/time.h>

#include <engine/console.h>
#include <engine/server/databases/connection.h>
#include <engine/server/sql_string_helpers.h>
#include <engine/shared/config.h>

#include <generated/insta/mode_account.h>

#include <game/server/player.h>

#include <insta/server/db/accounts.h>
#include <insta/server/db/stats.h>
#include <insta/server/display_name.h>

#include <algorithm>

void CGameControllerInstaCore::EnableAccTable(EExtraAccTable Table)
{
	if(!m_pExtraAccountTableController)
	{
		m_pExtraAccountTableController = new CExtraAccountTableController();
	}

	bool Duplicate = std::find(
				 m_pExtraAccountTableController->m_vTables.begin(),
				 m_pExtraAccountTableController->m_vTables.end(),
				 Table) != m_pExtraAccountTableController->m_vTables.end();
	dbg_assert(Duplicate == false, "tried to enable account extra table %d more than once", (int)Table);
	m_pExtraAccountTableController->m_vTables.emplace_back(Table);
}

void CGameControllerInstaCore::CreateAccountsTable()
{
	Db()->Accounts()->CreateTable();

	if(m_pExtraAccountTableController)
	{
		log_info("accounts", "creating %" PRIzu " additional account tables ..", m_pExtraAccountTableController->m_vTables.size());
		Db()->Stats()->CreateExtraAccountsTables(m_pExtraAccountTableController->m_vTables);
	}
}

void CGameControllerInstaCore::OnLogin(const CAccount *pAccount, class CPlayer *pPlayer)
{
	if(!g_Config.m_SvAccounts)
	{
		// TODO: this should be hard to hit right? Its basically a race condition with the worker thread.
		//       but if it does get hit the account will stay logged in in the database that is bad
		//       we do logout all in the db on deactivate does that already work?
		//       is the sql queue executed in order? Do we need to schedule a db worker to logout this account?
		//       or login the account properly on the player instance and trigger a logout instantly
		GameServer()->SendChatTarget(pPlayer->GetCid(), "Login failed. Accounts were turned off by an admin");
		return;
	}

	GameServer()->SendChatTarget(pPlayer->GetCid(), "Successfully logged in");
	log_info(
		"accounts",
		"cid=%d addr=<{%s}> name='%s' username='%s' logged in",
		pPlayer->GetCid(),
		Server()->ClientAddrString(pPlayer->GetCid(), true),
		Server()->ClientName(pPlayer->GetCid()),
		pAccount->Username());

	pPlayer->m_Account = *pAccount;
	pPlayer->m_Account.m_IsLoggedIn = true;

	bool NameWouldChange = str_comp(Server()->ClientName(pPlayer->GetCid()), pPlayer->m_DisplayName.WantedName());
	bool CouldNotUseName = !pPlayer->m_DisplayName.CanUseName();
	pPlayer->m_DisplayName.SetAccountUsername(pAccount->Username());

	if(pPlayer->m_DisplayName.CanUseName() && NameWouldChange && CouldNotUseName)
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
			pPlayer->m_DisplayName.NumChanges() == 1,
			/*
			 * but do inform 0.7 clients about the change
			 * so they can show the new name
			 */
			true);
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

	log_info(
		"accounts",
		"cid=%d name='%s' username='%s' logged out: %s",
		pPlayer->GetCid(),
		Server()->ClientName(pPlayer->GetCid()),
		pPlayer->m_Account.Username(),
		pSuccessMessage);

	Db()->Accounts()->SaveAndLogout(pPlayer, pSuccessMessage);
	pPlayer->m_DisplayName.SetAccountUsername("");
	if(!pPlayer->m_DisplayName.CanUseName())
	{
		GameServer()->SendChatTarget(pPlayer->GetCid(), "You can no longer use this name because you logged out");
		GameServer()->ChangeName(pPlayer->GetCid(), pPlayer->m_DisplayName.DisplayName(), false, true);
	}
}

void CGameControllerInstaCore::OnLogout(class CPlayer *pPlayer, const char *pMessage)
{
	pPlayer->m_Account.Reset();
	pPlayer->m_Account.m_IsLoggedIn = false;
	SendChatTarget(pPlayer->GetCid(), pMessage);
}

void CGameControllerInstaCore::LogoutAllAccounts(const char *pSuccessMessage)
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		LogoutAccount(pPlayer, pSuccessMessage);
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

	Db()->Accounts()->LogoutAllOnCurrentServer();
}

void CGameControllerInstaCore::OnBeforeReload()
{
	// if we are about to switch to the pure "ddnet"
	// gametype that has no account support we need to logout all
	// users in the database first
	CheckLogoutNeededBeforeGametypeSwitch();
}

void CGameControllerInstaCore::CheckLogoutNeededBeforeGametypeSwitch()
{
	if(!g_Config.m_SvAccounts)
		return;

	bool IsUnknownGametype = true;
	for(const auto &[String, _] : Gamemodes())
	{
		if(str_comp_nocase(Config()->m_SvGametype, String.c_str()) == 0)
		{
			IsUnknownGametype = false;
			break;
		}
	}

	// this unknown fallback has to be kept in sync
	// with the code in gametcontext.cpp
	// if we ever change the default to not be pure "ddnet"
	// we do not need to logout all anymore
	if(str_comp_nocase(Config()->m_SvGametype, "ddnet") == 0 || IsUnknownGametype)
	{
		log_warn("accounts", "about to switch to 'ddnet' mode, logging out all users");
		LogoutAllAccounts("Logged out of account (switching to ddnet gametype)");
	}
}

void CGameControllerInstaCore::RequestChangePassword(class CPlayer *pPlayer, const char *pOldPassword, const char *pNewPassword)
{
	dbg_assert(pPlayer->m_Account.IsLoggedIn(), "player without active account tried to change password");
	Db()->Accounts()->ChatCmd(
		pPlayer->GetCid(),
		pPlayer->m_Account.m_aUsername,
		Server()->ClientName(pPlayer->GetCid()),
		pOldPassword,
		pNewPassword,
		EAccountChatCmd::CHAT_CMD_CHANGE_PASSWORD);
}

void CGameControllerInstaCore::OnChangePassword(class CPlayer *pPlayer)
{
	SendChatTarget(pPlayer->GetCid(), "Password updated successfully");
}

void CGameControllerInstaCore::OnFailedAccountLogin(class CPlayer *pPlayer, const char *pErrorMsg, const char *pUsername)
{
	SendChatTarget(pPlayer->GetCid(), pErrorMsg);
	if(CIpRatelimit::TrackWrongLogin(m_vIpRatelimits, Server()->ClientAddr(pPlayer->GetCid()), Server()->Tick()))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' failed too many account logins and got ratelimited", Server()->ClientName(pPlayer->GetCid()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
	}

	log_info(
		"accounts",
		"cid=%d addr=<{%s}> name='%s' username='%s' failed login: %s",
		pPlayer->GetCid(),
		Server()->ClientAddrString(pPlayer->GetCid(), true),
		Server()->ClientName(pPlayer->GetCid()),
		pUsername,
		pErrorMsg);
}

void CGameControllerInstaCore::ChatCmdDisplayName(CPlayer *pPlayer)
{
	Db()->Accounts()->ChatCmd(
		pPlayer->GetCid(),
		pPlayer->m_Account.m_aUsername,
		Server()->ClientName(pPlayer->GetCid()),
		"",
		"",
		EAccountChatCmd::CHAT_CMD_DISPLAY_NAME);
}

void CGameControllerInstaCore::ChatCmdLockName(CPlayer *pPlayer)
{
	Db()->Accounts()->ChatCmd(
		pPlayer->GetCid(),
		pPlayer->m_Account.m_aUsername,
		pPlayer->m_Account.m_aDisplayName,
		"",
		"",
		EAccountChatCmd::CHAT_CMD_LOCK_NAME);
}

void CGameControllerInstaCore::OnDisplayNameSet(CPlayer *pPlayer, const char *pDisplayName, const char *pUsername)
{
	// the player can logout or switch accounts
	// while the name claim is pending
	// in that case we drop this event
	if(!pPlayer->m_Account.IsLoggedIn())
		return;
	if(str_comp(pPlayer->m_Account.Username(), pUsername))
		return;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "You set '%s' as your display name. Only your account can use it now.", pDisplayName);
	GameServer()->SendChatTarget(pPlayer->GetCid(), aBuf);

	str_copy(pPlayer->m_Account.m_aDisplayName, pDisplayName);

	CDisplayNameOwner Owner;
	str_copy(Owner.m_aUsername, pPlayer->m_Account.Username());
	str_copy(Owner.m_aDisplayName, pDisplayName);
	Owner.m_IsProtected = true;

	// the player could have performed a name change
	// while the name claim was pending
	if(!str_comp(pPlayer->m_DisplayName.WantedName(), pDisplayName))
		pPlayer->m_DisplayName.SetNameOwner(&Owner);
}

void CGameControllerInstaCore::OnNameLocked(CPlayer *pPlayer, const char *pDisplayName, const char *pUsername, bool IsProtected)
{
	// the player can logout or switch accounts
	// while the name claim is pending
	// in that case we drop this event
	if(!pPlayer->m_Account.IsLoggedIn())
		return;
	if(str_comp(pPlayer->m_Account.Username(), pUsername))
		return;

	char aBuf[512];
	if(IsProtected)
	{
		str_format(
			aBuf,
			sizeof(aBuf),
			"You protected the name '%s'. Only your account can use it now.",
			pDisplayName);
	}
	else
	{
		str_format(
			aBuf,
			sizeof(aBuf),
			"You unlocked the name '%s'. Everybody can use the name now, but your account still owns it. So nobody can take it away.",
			pDisplayName);
	}
	GameServer()->SendChatTarget(pPlayer->GetCid(), aBuf);

	str_copy(pPlayer->m_Account.m_aDisplayName, pDisplayName);

	CDisplayNameOwner Owner;
	str_copy(Owner.m_aUsername, pPlayer->m_Account.Username());
	str_copy(Owner.m_aDisplayName, pDisplayName);
	Owner.m_IsProtected = IsProtected;

	// the player could have performed a name change
	// while the name claim was pending
	if(!str_comp(pPlayer->m_DisplayName.WantedName(), pDisplayName))
		pPlayer->m_DisplayName.SetNameOwner(&Owner);
}

bool CGameControllerInstaCore::IsAccountRatelimited(int ClientId, char *pReason, int ReasonSize)
{
	if(pReason)
		pReason[0] = '\0';

	// econ and fifo have no ratelimits
	if(ClientId == IConsole::CLIENT_ID_UNSPECIFIED)
		return false;

	// rate limit if delay to last query is too short
	if(Db()->IsRateLimitedPlayer(ClientId))
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
	Db()->Accounts()->RconCmd(ClientId, pUsername, pPassword, EAccountRconCmd::ACC_SET_PASSWORD);
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

	Db()->Accounts()->RconCmd(ClientId, pUsername, "", EAccountRconCmd::ACC_LOGOUT);
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
	Db()->Accounts()->RconCmd(ClientId, pUsername, "", EAccountRconCmd::ACC_LOCK);
}

void CGameControllerInstaCore::RconUnlockAccount(int ClientId, const char *pUsername)
{
	Db()->Accounts()->RconCmd(ClientId, pUsername, "", EAccountRconCmd::ACC_UNLOCK);
}

void CGameControllerInstaCore::RconAccountInfo(int ClientId, const char *pUsername)
{
	Db()->Accounts()->RconCmd(ClientId, pUsername, "", EAccountRconCmd::ACC_INFO);
}

void CGameControllerInstaCore::RconAccountStatus(int ClientId)
{
	char aHostname[512];
	GameServer()->GetHostname(aHostname, sizeof(aHostname));
	log_info("accounts", "account system information:");
	log_info("accounts", " server MySQL support: %s", MysqlAvailable() ? "AVAILABLE" : "UNAVAILABLE");
	log_info("accounts", " accounts db backend active: %s", (Config()->m_SvUseSql && MysqlAvailable()) ? "MySQL" : "sqlite3");
	log_info("accounts", " hostname: %s", aHostname);
	log_info("accounts", " port: %d", GameServer()->m_ServerPortOnLaunch);
	if(m_pExtraAccountTableController)
	{
		CExtraAccountTableController *pExt = m_pExtraAccountTableController;
		log_info("accounts", " extra tables loaded by mode: %" PRIzu, pExt->m_vTables.size());
		for(auto Table : pExt->m_vTables)
		{
			log_info("accounts", "  - %s", CExtraAccountTableController::EnumToTableName(Table));
		}
	}
	else
	{
		log_info("accounts", " extra tables loaded by mode: 0");
	}

	// TODO: also show other stats here like:
	//       - amount of account registrations on this server
	//         no db query needed just track every signup in a variable
	//       - amount of failed logins on this server (also variable)
	//       - amount of total accounts (db query needed but can be cached)
	//         depends on this to be solved
	//         https://github.com/ddnet-insta/ddnet-insta/pull/667
	//       - maybe also some performance stats like login time
}

void CGameControllerInstaCore::RconAccountRatelimits(int AdminClientId, int VictimClientId, const char *pCommand)
{
	CIpRatelimit::RconCmdLogOrReset(
		m_vIpRatelimits,
		Server()->ClientAddr(VictimClientId),
		Server()->Tick(),
		pCommand);
}

void CGameControllerInstaCore::OnAccountInfo(int AdminUniqueClientId, const char *pUsername, CAccount *pAccount)
{
	log_info("account", "account '%s'", pUsername);
	log_info("account", " id: %d", pAccount->Id());
	CPlayer *pVictim = GetPlayerByAccountUsername(pUsername);
	if(pVictim)
		log_info("account", " connected on this server cid=%d name='%s'", pVictim->GetCid(), Server()->ClientName(pVictim->GetCid()));
	else if(pAccount->m_IsLoggedIn)
		log_info("account", " connected on %s:%d", pAccount->m_aServerIp, pAccount->m_ServerPort);
	else
		log_info("account", " not connected (last seen on %s:%d)", pAccount->m_aServerIp, pAccount->m_ServerPort);

	log_info("account", " locked: %d", pAccount->IsLocked());
	log_info("account", " contact: %s", pAccount->m_aContact);

	char aDate[512];
	str_timestamp_ex(pAccount->m_RegisterDate, aDate, sizeof(aDate), TimestampFormat::SPACE);
	log_info("account", " register date: %s", aDate);

	if(pAccount->m_LastLogin.has_value())
		str_timestamp_ex(pAccount->m_LastLogin.value(), aDate, sizeof(aDate), TimestampFormat::SPACE);
	else
		str_copy(aDate, "never");
	log_info("account", " last login date: %s", aDate);
	log_info("account", " pin: %s", pAccount->m_Pin.has_value() ? "set" : "unset");
	log_info("account", " display name: %s", pAccount->m_aDisplayName);
	if(pAccount->m_aDisplayName[0])
	{
		log_info("account", " display name locked: %s", pAccount->m_IsNameProtected ? "yes" : "no");
	}
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
	case EAccountRconCmd::LOG_INFO:
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			log_info("ddnet-insta", "%s", aMessage);
		}
		break;
	case EAccountRconCmd::LOG_ERROR:
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			log_error("ddnet-insta", "%s", aMessage);
		}
		break;
	case EAccountRconCmd::DIRECT:
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
	case EAccountRconCmd::ALL:
	{
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;

			GameServer()->SendChat(-1, TEAM_ALL, aMessage, -1);
		}
		break;
	}
	case EAccountRconCmd::ACC_SET_PASSWORD:
		dbg_assert(false, "Set password used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconCmd::ACC_LOGOUT:
		dbg_assert(false, "Logout used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconCmd::ACC_LOCK:
		dbg_assert(false, "Lock used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconCmd::ACC_UNLOCK:
		dbg_assert(false, "Unlock used as result type. Expected log info or log error instead.");
		break;
	case EAccountRconCmd::ACC_INFO:
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
		LogoutAllAccounts("Logged out of account (account system deactivated)");
	}
}
