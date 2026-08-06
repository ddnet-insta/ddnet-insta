#include "accounts.h"

#include <base/log.h>
#include <base/time.h>

#include <engine/shared/config.h>

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>
#include <insta/server/db/accounts_worker/accounts_worker.h>

CDbAccounts::CDbAccounts(CGameContext *pGameServer, CDbConnectionPool *pPool, CDbInsta *pInstaDatabase) :
	m_pPool(pPool),
	m_pGameServer(pGameServer),
	m_pServer(pGameServer->Server()),
	m_pInstaDatabase(pInstaDatabase)
{
}

void CDbAccounts::CreateTable()
{
	auto Tmp = std::make_unique<CSqlCreateTableRequest>();
	Tmp->m_aName[0] = '\0';
	Tmp->m_aColumns[0] = '\0';
	Tmp->m_aColumns[0] = '\0';
	m_pPool->ExecuteWrite(CAccountsWorker::CreateAccountsTableThread, std::move(Tmp), "create accounts table");
}

std::shared_ptr<CAccountPlayerResult> CDbAccounts::NewPlayerResult(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(pPlayer->m_AccountQueryResult != nullptr)
	{
		// This should never be hit. We want to show a more useful error to the user further
		// up in the stack instead of silently failing here.
		log_error("sql", "cid=%d failed to launch sql query because there is already one pending", ClientId);
		return nullptr;
	}
	pPlayer->m_AccountQueryResult = std::make_shared<CAccountPlayerResult>();
	return pPlayer->m_AccountQueryResult;
}

void CDbAccounts::ExecPlayerThreadRatelimited(
	bool (*pFuncPtr)(IDbConnection *, const ISqlData *, Write w, char *pError, int ErrorSize),
	const char *pThreadName,
	int ClientId,
	const char *pUsername,
	const char *pDisplayName,
	const char *pOldPassword,
	const char *pNewPassword,
	EAccountChatCmd RequestType)
{
	auto pResult = NewPlayerResult(ClientId);
	if(pResult == nullptr)
		return;

	auto Tmp = std::make_unique<CSqlPlayerAccountRequest>(pResult, g_Config.m_SvDebugStats);
	Tmp->m_ClientId = ClientId;
	str_copy(Tmp->m_aUsername, pUsername, sizeof(Tmp->m_aUsername));
	str_copy(Tmp->m_aDisplayName, pDisplayName, sizeof(Tmp->m_aDisplayName));
	str_copy(Tmp->m_aOldPassword, pOldPassword, sizeof(Tmp->m_aOldPassword));
	str_copy(Tmp->m_aNewPassword, pNewPassword, sizeof(Tmp->m_aNewPassword));
	str_timestamp_format(Tmp->m_aTimestamp, sizeof(Tmp->m_aTimestamp), TimestampFormat::SPACE); // 2019-04-02 19:41:58
	GameServer()->GetHostname(Tmp->m_aServerIp, sizeof(Tmp->m_aServerIp));
	Tmp->m_ServerPort = GameServer()->m_ServerPortOnLaunch;
	Tmp->m_RequestType = RequestType;
	str_copy(Tmp->m_aUserIpAddr, Server()->ClientAddrString(ClientId, false), sizeof(Tmp->m_aUserIpAddr));

	Tmp->m_vTables.clear();
	if(GameServer()->m_pController->m_pExtraAccountTableController)
	{
		Tmp->m_vTables = GameServer()->m_pController->m_pExtraAccountTableController->m_vTables;
	}

	m_pPool->ExecuteWrite(pFuncPtr, std::move(Tmp), pThreadName);
}

bool CDbAccounts::IsRatelimitError(int ClientId)
{
	if(Db()->RateLimitPlayer(ClientId))
	{
		// this should never happen!
		// the ratelimit check should already have been applied
		// and shown a useful error message to the user
		// if this gets printed there is a flaw in the code that called it!
		log_error("sql", "FATAL ERROR: cid=%d got ratelimited trying to perform an account action", ClientId);
		return true;
	}

	CPlayer *pCurPlayer = GameServer()->m_apPlayers[ClientId];
	if(pCurPlayer->m_AccountQueryResult != nullptr)
	{
		// this should never happen!
		// the ratelimit check should already have been applied
		// and shown a useful error message to the user
		// if this gets printed there is a flaw in the code that called it!

		// should we assert here and crash the entire server?
		// can we even do that from a thread?
		// if this gets hit it is really bad!
		log_error("sql", "FATAL ERROR: player already had an account operation pending. Ignoring this new one!");
		return true;
	}
	return false;
}

void CDbAccounts::ChatCmd(int ClientId, const char *pUsername, const char *pDisplayName, const char *pOldPassword, const char *pNewPassword, EAccountChatCmd RequestType)
{
	if(IsRatelimitError(ClientId))
		return;

	ExecPlayerThreadRatelimited(CAccountsWorker::ChatCmdWorker, "account", ClientId, pUsername, pDisplayName, pOldPassword, pNewPassword, RequestType);
}

// FIXME: provide one generic method available for mods that takes a lamdba as argument
//        where mods can pass their own struct
//        it might get a bit long and messy but then they can do that without having to edit
//        any ddnet-insta code which is amazing
//
//        i wonder how the sync on the main thread will be handled again
//        can we pass a second lambda that will run on the main thread?
//
//        where did i do this already? in ddnet++?

void CDbAccounts::ChatCmdSlowOperation(int ClientId)
{
	if(IsRatelimitError(ClientId))
		return;

	auto pResult = NewPlayerResult(ClientId);
	if(pResult == nullptr)
		return;

	struct CSlowReq : CSqlAccData
	{
		CSlowReq(std::shared_ptr<CAccountPlayerResult> pResult) :
			CSqlAccData(std::move(pResult))
		{
		}
		char m_aUsername[MAX_NAME_LENGTH];
	};
	auto Tmp = std::make_unique<CSlowReq>(pResult);
	str_copy(Tmp->m_aUsername, "yolo", sizeof(Tmp->m_aUsername));

	m_pPool->ExecuteWrite(
		+[](IDbConnection *pConn, const ISqlData *pGameData, Write w, char *pError, int ErrorSize) -> bool
		{
			if(w != Write::NORMAL)
			{
				// could write to backup database here
				return true;
			}

			const auto *pData = dynamic_cast<const CSlowReq*>(pGameData);
			auto *pResult = dynamic_cast<CAccountPlayerResult *>(pGameData->m_pResult.get());

			log_info("sql-thread", "starting slooooooooooooooooooooooooooooooooow debug operation ...");
			log_info("sql-thread", "yo we are in a lambda pog");
			log_info("sql-thread", "username passed in: %s", pData->m_aUsername);
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(10000ms);
			log_info("sql-thread", "finished slow debug operation");
			pResult->m_MessageKind = EAccountChatCmd::CHAT_CMD_SLOW_ACCOUNT_OPERATION;
			str_copy(pResult->m_Data.m_aaMessages[0], "slow debug operation reached main thread");
			return true;
		},
		std::move(Tmp),
		"acc_slow_op");
}

void CDbAccounts::RconCmd(int ClientId, const char *pUsername, const char *pPassword, EAccountRconCmd RequestType)
{
	if(!GameServer()->m_pController)
	{
		log_error("sql", "FATAL ERROR: can not execute account rcon command during map change.");
		return;
	}
	if(GameServer()->m_pController->IsAccountRconCmdRatelimited(ClientId, nullptr, 0))
	{
		// this should never be hit!
		// who ever calls this method should check it first!
		log_error("sql", "FATAL ERROR: can not execute rcon command. Uncaught ratelimit!");
		return;
	}
	CPlayer *pPlayer = GameServer()->m_pController->GetPlayerOrNullptr(ClientId);
	// econ and fifo have no client id but they can still manage accounts using rcon commands
	// TODO: use unspecified variable if this gets merged https://github.com/ddnet/ddnet/pull/11434
	uint32_t UniqueClientId = pPlayer == nullptr ? 0 : pPlayer->GetUniqueCid();

	std::shared_ptr<CAccountRconCmdResult> pResult = std::make_shared<CAccountRconCmdResult>(UniqueClientId);
	GameServer()->m_vAccountRconCmdQueryResults.emplace_back(pResult);

	auto Tmp = std::make_unique<CSqlPlayerAccountRconCmdData>(pResult, g_Config.m_SvDebugStats);
	Tmp->m_RequestType = RequestType;
	str_copy(Tmp->m_aAdminName, Server()->ClientName(ClientId));
	str_copy(Tmp->m_aPassword, pPassword);
	str_copy(Tmp->m_aUsername, pUsername);
	GameServer()->GetHostname(Tmp->m_aServerIp, sizeof(Tmp->m_aServerIp));
	Tmp->m_ServerPort = GameServer()->m_ServerPortOnLaunch;
	m_pPool->ExecuteWrite(CAccountsWorker::RconCmdWorker, std::move(Tmp), "rcon cmd");
}

void CDbAccounts::SaveAndLogout(CPlayer *pPlayer, const char *pSuccessMessage)
{
	// we do not consider it an error if two logouts were scheduled in a row
	// both have the same desired end result
	// both should have the same state
	// so dropping one of them is fine
	//
	// this can happen if a players does /logout and then instantly disconnects
	// which is fine!
	if(pPlayer->m_AccountLogoutQueryResult != nullptr)
	{
		// it should still only happen rarely
		// and if this keeps looping we know something is up so lets log a warning here
		// the user calling /logout twice should be dropped on chat command level already
		//
		// this can happen when a player uses the /logout command right before disconnecting or server shutdown
		log_warn(
			"sql",
			"tried to logout account '%s' but player '%s' already has a logout operation pending!",
			pPlayer->m_Account.m_aUsername, Server()->ClientName(pPlayer->GetCid()));
		return;
	}

	pPlayer->m_AccountLogoutQueryResult = std::make_shared<CAccountManagementResult>(pSuccessMessage);
	auto Tmp = std::make_unique<CSqlPlayerAccountData>(pPlayer->m_AccountLogoutQueryResult, g_Config.m_SvDebugStats);
	Tmp->m_Account = pPlayer->m_Account;
	Tmp->m_vTables.clear();
	if(GameServer()->m_pController->m_pExtraAccountTableController)
	{
		Tmp->m_vTables = GameServer()->m_pController->m_pExtraAccountTableController->m_vTables;
	}
	m_pPool->ExecuteWrite(CAccountsWorker::AccountSaveAndLogoutWorker, std::move(Tmp), "save and logout");
}

void CDbAccounts::LogoutAllOnCurrentServer()
{
	auto Tmp = std::make_unique<CSqlLogoutAllRequest>();
	GameServer()->GetHostname(Tmp->m_aServerIp, sizeof(Tmp->m_aServerIp));
	Tmp->m_ServerPort = GameServer()->m_ServerPortOnLaunch;
	m_pPool->ExecuteWrite(CAccountsWorker::LogoutAllAccountsOnCurrentServerThread, std::move(Tmp), "logout all");
}

bool CDbAccounts::CheckNameClaimed(int ClientId, const char *pName)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(pPlayer->m_CheckClaimNameQueryResult != nullptr)
		return false;
	pPlayer->m_CheckClaimNameQueryResult = std::make_shared<CCheckNameClaimResult>();

	auto pResult = pPlayer->m_CheckClaimNameQueryResult;
	auto Tmp = std::make_unique<CSqlCheckNameClaimRequest>(pResult);
	str_copy(Tmp->m_aDisplayName, pName);
	m_pPool->Execute(CAccountsWorker::CheckNameClaimedWorker, std::move(Tmp), "check name claimed");
	return true;
}
