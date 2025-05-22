#include <base/log.h>
#include <base/system.h>
#include <cstdlib>
#include <engine/server/databases/connection.h>
#include <engine/server/databases/connection_pool.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/extra_columns.h>
#include <game/server/instagib/sql_stats_player.h>
#include <game/server/player.h>

#include "sql_stats.h"

class IDbConnection;

CInstaSqlResult::CInstaSqlResult()
{
	SetVariant(EInstaSqlRequestType::DIRECT);
	m_Stats.Reset();
	m_Rank = 0;
}

void CInstaSqlResult::SetVariant(EInstaSqlRequestType RequestType)
{
	m_MessageKind = RequestType;
	switch(RequestType)
	{
	case EInstaSqlRequestType::CHAT_CMD_RANK:
	case EInstaSqlRequestType::CHAT_CMD_STATSALL:
	case EInstaSqlRequestType::CHAT_CMD_MULTIS:
	case EInstaSqlRequestType::PLAYER_DATA:
	case EInstaSqlRequestType::DIRECT:
	case EInstaSqlRequestType::ALL:
		for(auto &aMessage : m_aaMessages)
			aMessage[0] = 0;
		break;
	case EInstaSqlRequestType::BROADCAST:
		m_aBroadcast[0] = 0;
		break;
		break;
	}
}

// hack to avoid editing connection.h in ddnet code
ESqlBackend CSqlStats::DetectBackend(IDbConnection *pSqlServer)
{
	return str_comp(pSqlServer->BinaryCollate(), "BINARY") == 0 ? ESqlBackend::SQLITE3 : ESqlBackend::MYSQL;
}

std::shared_ptr<CInstaSqlResult> CSqlStats::NewInstaSqlResult(int ClientId)
{
	CPlayer *pCurPlayer = GameServer()->m_apPlayers[ClientId];
	if(pCurPlayer->m_StatsQueryResult != nullptr) // TODO: send player a message: "too many requests"
		return nullptr;
	pCurPlayer->m_StatsQueryResult = std::make_shared<CInstaSqlResult>();
	return pCurPlayer->m_StatsQueryResult;
}

// this shares one ratelimit with ddnet based requests such as /rank, /times, /top5team and so on
bool CSqlStats::RateLimitPlayer(int ClientId)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(pPlayer == 0)
		return true;
	if(pPlayer->m_LastSqlQuery + (int64_t)g_Config.m_SvSqlQueriesDelay * Server()->TickSpeed() >= Server()->Tick())
		return true;
	pPlayer->m_LastSqlQuery = Server()->Tick();
	return false;
}

void CSqlStats::ExecPlayerStatsThread(
	bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
	const char *pThreadName,
	int ClientId,
	const char *pName,
	const char *pTable,
	EInstaSqlRequestType RequestType)
{
	auto pResult = NewInstaSqlResult(ClientId);
	if(pResult == nullptr)
		return;
	auto Tmp = std::make_unique<CSqlPlayerStatsRequest>(pResult, g_Config.m_SvDebugStats);
	str_copy(Tmp->m_aName, pName, sizeof(Tmp->m_aName));
	str_copy(Tmp->m_aRequestingPlayer, Server()->ClientName(ClientId), sizeof(Tmp->m_aRequestingPlayer));
	str_copy(Tmp->m_aTable, pTable, sizeof(Tmp->m_aTable));
	Tmp->m_RequestType = RequestType;

	if(m_pExtraColumns)
	{
		Tmp->m_pExtraColumns = (CExtraColumns *)malloc(sizeof(CExtraColumns));
		mem_copy(Tmp->m_pExtraColumns, m_pExtraColumns, sizeof(CExtraColumns));
		if(g_Config.m_SvDebugStats > 1)
			dbg_msg("sql", "allocated memory at %p", Tmp->m_pExtraColumns);
	}

	m_pPool->Execute(pFuncPtr, std::move(Tmp), pThreadName);
}

void CSqlStats::ExecPlayerRankOrTopThread(
	bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
	const char *pThreadName,
	int ClientId,
	const char *pName,
	const char *pRankColumnDisplay,
	const char *pRankColumnSql,
	const char *pTable,
	const char *pOrderBy,
	int Offset)
{
	auto pResult = NewInstaSqlResult(ClientId);
	if(pResult == nullptr)
		return;
	auto Tmp = std::make_unique<CSqlPlayerStatsRequest>(pResult, g_Config.m_SvDebugStats);
	str_copy(Tmp->m_aName, pName, sizeof(Tmp->m_aName));
	str_copy(Tmp->m_aRequestingPlayer, Server()->ClientName(ClientId), sizeof(Tmp->m_aRequestingPlayer));
	str_copy(Tmp->m_aRankColumnDisplay, pRankColumnDisplay, sizeof(Tmp->m_aRankColumnDisplay));
	str_copy(Tmp->m_aRankColumnSql, pRankColumnSql, sizeof(Tmp->m_aRankColumnSql));
	str_copy(Tmp->m_aTable, pTable, sizeof(Tmp->m_aTable));
	str_copy(Tmp->m_aOrderBy, pOrderBy, sizeof(Tmp->m_aOrderBy));
	Tmp->m_Offset = Offset;

	if(m_pExtraColumns)
	{
		Tmp->m_pExtraColumns = (CExtraColumns *)malloc(sizeof(CExtraColumns));
		mem_copy(Tmp->m_pExtraColumns, m_pExtraColumns, sizeof(CExtraColumns));
		if(g_Config.m_SvDebugStats > 1)
			dbg_msg("sql", "allocated memory at %p", Tmp->m_pExtraColumns);
	}

	m_pPool->Execute(pFuncPtr, std::move(Tmp), pThreadName);
}

void CSqlStats::ExecPlayerFastcapRankOrTopThread(
	bool (*pFuncPtr)(IDbConnection *, const ISqlData *, char *pError, int ErrorSize),
	const char *pThreadName,
	int ClientId,
	const char *pName,
	const char *pMap,
	const char *pGametype,
	bool Grenade,
	bool OnlyStatTrack,
	int Offset)
{
	auto pResult = NewInstaSqlResult(ClientId);
	if(pResult == nullptr)
		return;
	auto Tmp = std::make_unique<CSqlPlayerFastcapRequest>(pResult, g_Config.m_SvDebugStats);

	str_copy(Tmp->m_aName, pName, sizeof(Tmp->m_aName));
	str_copy(Tmp->m_aRequestingPlayer, Server()->ClientName(ClientId), sizeof(Tmp->m_aRequestingPlayer));
	Tmp->m_Offset = Offset;
	str_copy(Tmp->m_aGametype, pGametype, sizeof(Tmp->m_aGametype));
	str_copy(Tmp->m_aMap, pMap, sizeof(Tmp->m_aMap));
	Tmp->m_Grenade = Grenade;
	Tmp->m_OnlyStatTrack = OnlyStatTrack;

	m_pPool->Execute(pFuncPtr, std::move(Tmp), pThreadName);
}

CSqlStats::CSqlStats(CGameContext *pGameServer, CDbConnectionPool *pPool) :
	m_pPool(pPool),
	m_pGameServer(pGameServer),
	m_pServer(pGameServer->Server())
{
}

void CSqlStats::SetExtraColumns(CExtraColumns *pExtraColumns)
{
	m_pExtraColumns = pExtraColumns;
}

CSqlInstaData::~CSqlInstaData()
{
	if(m_DebugStats > 1)
		dbg_msg("sql-thread", "round stats request destructor called");

	if(m_pExtraColumns)
	{
		if(m_DebugStats > 1)
			dbg_msg("sql-thread", "free memory at %p", m_pExtraColumns);
		free(m_pExtraColumns);
		m_pExtraColumns = nullptr;
	}
}

void CSqlStats::LoadInstaPlayerData(int ClientId, const char *pTable)
{
	const char *pName = Server()->ClientName(ClientId);
	ExecPlayerStatsThread(ShowStatsWorker, "load insta player data", ClientId, pName, pTable, EInstaSqlRequestType::PLAYER_DATA);
}

void CSqlStats::ShowStats(int ClientId, const char *pName, const char *pTable, EInstaSqlRequestType RequestType)
{
	if(RateLimitPlayer(ClientId))
		return;
	ExecPlayerStatsThread(ShowStatsWorker, "show stats", ClientId, pName, pTable, RequestType);
}

void CSqlStats::ShowRank(
	int ClientId,
	const char *pName,
	const char *pRankColumnDisplay,
	const char *pRankColumnSql,
	const char *pTable,
	const char *pOrderBy)
{
	if(RateLimitPlayer(ClientId))
		return;
	ExecPlayerRankOrTopThread(
		ShowRankWorker,
		"show rank",
		ClientId,
		pName,
		pRankColumnDisplay,
		pRankColumnSql,
		pTable,
		pOrderBy,
		0);
}

void CSqlStats::ShowTop(
	int ClientId,
	const char *pName,
	const char *pRankColumnDisplay,
	const char *pRankColumnSql,
	const char *pTable,
	const char *pOrderBy,
	int Offset)
{
	if(RateLimitPlayer(ClientId))
		return;
	ExecPlayerRankOrTopThread(
		ShowTopWorker,
		"show top",
		ClientId,
		pName,
		pRankColumnDisplay,
		pRankColumnSql,
		pTable,
		pOrderBy,
		Offset);
}

void CSqlStats::ShowFastcapRank(
	int ClientId,
	const char *pName,
	const char *pMap,
	const char *pGametype,
	bool Grenade,
	bool OnlyStatTrack)
{
	if(RateLimitPlayer(ClientId))
		return;
	ExecPlayerFastcapRankOrTopThread(
		ShowFastcapRankWorker,
		"show fastcap rank",
		ClientId,
		pName,
		pMap,
		pGametype,
		Grenade,
		OnlyStatTrack,
		0);
}

void CSqlStats::ShowFastcapTop(
	int ClientId,
	const char *pName,
	const char *pMap,
	const char *pGametype,
	bool Grenade,
	bool OnlyStatTrack,
	int Offset)
{
	if(RateLimitPlayer(ClientId))
		return;

	ExecPlayerFastcapRankOrTopThread(
		ShowFastcapTopWorker,
		"show fastcap top",
		ClientId,
		pName,
		pMap,
		pGametype,
		Grenade,
		OnlyStatTrack,
		Offset);
}

void CSqlStats::SaveRoundStats(const char *pName, const char *pTable, CSqlStatsPlayer *pStats)
{
	auto Tmp = std::make_unique<CSqlSaveRoundStatsData>(g_Config.m_SvDebugStats);

	if(m_pExtraColumns)
	{
		Tmp->m_pExtraColumns = (CExtraColumns *)malloc(sizeof(CExtraColumns));
		mem_copy(Tmp->m_pExtraColumns, m_pExtraColumns, sizeof(CExtraColumns));
		if(g_Config.m_SvDebugStats > 1)
			dbg_msg("sql", "allocated memory at %p", Tmp->m_pExtraColumns);
	}

	str_copy(Tmp->m_aName, pName);
	str_copy(Tmp->m_aTable, pTable);
	mem_copy(&Tmp->m_Stats, pStats, sizeof(Tmp->m_Stats));
	m_pPool->ExecuteWrite(SaveRoundStatsThread, std::move(Tmp), "save round stats");
}

void CSqlStats::SaveFastcap(int ClientId, int TimeTicks, const char *pTimestamp, bool Grenade, bool StatTrack)
{
	CPlayer *pCurPlayer = GameServer()->m_apPlayers[ClientId];
	if(pCurPlayer->m_FastcapQueryResult != nullptr)
		dbg_msg("sql", "WARNING: previous save fastcap result didn't complete, overwriting it now");

	pCurPlayer->m_FastcapQueryResult = std::make_shared<CInstaSqlResult>();
	auto Tmp = std::make_unique<CSqlPlayerFastcapData>(pCurPlayer->m_FastcapQueryResult, g_Config.m_SvDebugStats);
	str_copy(Tmp->m_aMap, Server()->GetMapName(), sizeof(Tmp->m_aMap));
	str_copy(Tmp->m_aGametype, GameServer()->m_pController->m_pGameType, sizeof(Tmp->m_aGametype));
	FormatUuid(GameServer()->GameUuid(), Tmp->m_aGameUuid, sizeof(Tmp->m_aGameUuid));
	Tmp->m_StatTrack = StatTrack;
	Tmp->m_Grenade = Grenade;
	str_copy(Tmp->m_aName, Server()->ClientName(ClientId), sizeof(Tmp->m_aName));
	Tmp->m_Time = (float)(TimeTicks) / (float)Server()->TickSpeed();
	str_copy(Tmp->m_aTimestamp, pTimestamp, sizeof(Tmp->m_aTimestamp));

	m_pPool->ExecuteWrite(SaveFastcapWorker, std::move(Tmp), "save fastcap");
}

bool CSqlStats::ShowStatsWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerStatsRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());

	// do not print "is unranked" in chat for new players
	// https://github.com/ddnet-insta/ddnet-insta/issues/245
	if(pData->m_RequestType == EInstaSqlRequestType::PLAYER_DATA)
		pResult->m_MessageKind = pData->m_RequestType;

	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT"
		" points, kills, deaths, spree,"
		" win_points, wins, losses, shots_fired, shots_hit %s "
		"FROM %s "
		"WHERE name = ?;",
		!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->SelectColumns(),
		pData->m_aTable);
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pData->m_aName);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed query: %s", aBuf);
		return false;
	}

	if(End)
	{
		str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]),
			"'%s' is unranked",
			pData->m_aName);
	}
	else
	{
		// success
		pResult->m_MessageKind = pData->m_RequestType;

		str_copy(pResult->m_Info.m_aRequestedPlayer, pData->m_aName, sizeof(pResult->m_Info.m_aRequestedPlayer));

		int Offset = 1;
		pResult->m_Stats.m_Points = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Kills = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Deaths = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_BestSpree = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_WinPoints = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Wins = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_Losses = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_ShotsFired = pSqlServer->GetInt(Offset++);
		pResult->m_Stats.m_ShotsHit = pSqlServer->GetInt(Offset++);

		if(pData->m_DebugStats > 1)
		{
			dbg_msg("sql-thread", "loaded base stats:");
			pResult->m_Stats.Dump(pData->m_pExtraColumns, "sql-thread");
		}

		CSqlStatsPlayer EmptyStats;
		EmptyStats.Reset();

		// merge into empty stats instead of having a ReadStats() and a ReadAndMergeStats() method
		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->ReadAndMergeStats(&Offset, pSqlServer, &pResult->m_Stats, &EmptyStats);

		if(pData->m_DebugStats > 1)
		{
			if(pData->m_pExtraColumns)
			{
				dbg_msg("sql-thread", "loaded gametype specific stats:");
				pResult->m_Stats.Dump(pData->m_pExtraColumns, "sql-thread");
			}
			else
			{
				dbg_msg("sql-thread", "warning no extra columns set!");
			}
		}
	}
	return true;
}

bool CSqlStats::ShowRankWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerStatsRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());

	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT %s, rank " // column
		"FROM (SELECT name, %s, RANK() OVER (ORDER BY %s %s) rank FROM %s) sub_table " // column, column, table
		"WHERE name = ?;",
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aOrderBy,
		pData->m_aTable);
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pData->m_aName);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed query: %s", aBuf);
		return false;
	}

	if(End)
	{
		str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]),
			"'%s' is unranked",
			pData->m_aName);
	}
	else
	{
		// success
		pResult->m_MessageKind = EInstaSqlRequestType::CHAT_CMD_RANK;

		str_copy(pResult->m_Info.m_aRequestedPlayer, pData->m_aName, sizeof(pResult->m_Info.m_aRequestedPlayer));
		str_copy(pResult->m_aRankColumnDisplay, pData->m_aRankColumnDisplay, sizeof(pResult->m_aRankColumnDisplay));

		pResult->m_RankedScore = pSqlServer->GetInt(1);
		pResult->m_Rank = pSqlServer->GetInt(2);
	}
	return true;
}

bool CSqlStats::ShowTopWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerStatsRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());

	auto *paMessages = pResult->m_aaMessages;

	int LimitStart = maximum(pData->m_Offset - 1, 0);

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf),
		"SELECT RANK() OVER (ORDER BY a.%s DESC) as ranking, %s, name "
		"FROM ("
		"  SELECT %s, name "
		"  FROM %s "
		"  ORDER BY %s DESC LIMIT ?"
		") as a "
		"ORDER BY ranking ASC, name ASC LIMIT ?, 5",
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aRankColumnSql,
		pData->m_aTable,
		pData->m_aRankColumnSql);
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare top failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindInt(1, LimitStart + 5);
	pSqlServer->BindInt(2, LimitStart);

	// show top points
	str_format(paMessages[0], sizeof(paMessages[0]), "-------- Top %s --------", pData->m_aRankColumnDisplay);
	bool End = false;
	int Line = 1;
	while(pSqlServer->Step(&End, pError, ErrorSize) && !End)
	{
		int Rank = pSqlServer->GetInt(1);
		int Points = pSqlServer->GetInt(2);
		char aName[MAX_NAME_LENGTH];
		pSqlServer->GetString(3, aName, sizeof(aName));
		str_format(paMessages[Line], sizeof(paMessages[Line]),
			"%d. '%s' - %s: %d", Rank, aName, pData->m_aRankColumnDisplay, Points);
		Line++;
	}
	if(!End)
	{
		return false;
	}
	str_copy(paMessages[Line], "-------------------------------", sizeof(paMessages[Line]));

	return true;
}

bool CSqlStats::ShowFastcapTopWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerFastcapRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());

	int LimitStart = maximum(absolute(pData->m_Offset) - 1, 0);
	const char *pOrder = pData->m_Offset >= 0 ? "ASC" : "DESC";

	if(pData->m_OnlyStatTrack)
	{
		dbg_msg("sql-thread", "WARNING! only stat track is not supported yet! Showing all flag times ...");
	}

	// check sort method
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf),
		"SELECT name, time, ranking "
		"FROM ("
		"  SELECT RANK() OVER w AS ranking, MIN(time) AS time, name "
		"  FROM fastcaps "
		"  WHERE map = ? "
		"  AND grenade = %s "
		"  GROUP BY name "
		"  WINDOW w AS (ORDER BY MIN(time))"
		") as a "
		"ORDER BY ranking %s "
		"LIMIT %d, ?",
		pData->m_Grenade ? pSqlServer->True() : pSqlServer->False(),
		pOrder,
		LimitStart);

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare top failed query: %s", aBuf);
		return false;
	}

	// TODO: grenade and stat track filters

	pSqlServer->BindString(1, pData->m_aMap);
	pSqlServer->BindInt(2, 5);

	// show top
	int Line = 0;
	str_copy(pResult->m_aaMessages[Line], "------------ Top flag times ------------", sizeof(pResult->m_aaMessages[Line]));

	Line++;

	char aTime[32];
	bool End = false;

	while(pSqlServer->Step(&End, pError, ErrorSize) && !End)
	{
		char aName[MAX_NAME_LENGTH];
		pSqlServer->GetString(1, aName, sizeof(aName));
		float Time = pSqlServer->GetFloat(2);
		str_time_float(Time, TIME_HOURS_CENTISECS, aTime, sizeof(aTime));
		int Rank = pSqlServer->GetInt(3);
		str_format(pResult->m_aaMessages[Line], sizeof(pResult->m_aaMessages[Line]),
			"%d. '%s' - Time: %s", Rank, aName, aTime);

		Line++;
	}

	str_copy(pResult->m_aaMessages[Line], "----------------------------------------------", sizeof(pResult->m_aaMessages[Line]));
	return End;
}

bool CSqlStats::ShowFastcapRankWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerFastcapRequest *>(pGameData);
	auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());

	if(pData->m_OnlyStatTrack)
	{
		dbg_msg("sql-thread", "WARNING! only stat track is not supported yet! Showing all flag times ...");
	}

	// check sort method
	char aBuf[600];
	str_format(aBuf, sizeof(aBuf),
		"SELECT ranking, time, percent_rank "
		"FROM ("
		"  SELECT RANK() OVER w AS ranking, PERCENT_RANK() OVER w as percent_rank, MIN(time) AS time, name "
		"  FROM fastcaps "
		"  WHERE map = ? "
		"  AND grenade = %s "
		"  GROUP BY name "
		"  WINDOW w AS (ORDER BY MIN(time))"
		") as a "
		"WHERE name = ?",
		pData->m_Grenade ? pSqlServer->True() : pSqlServer->False());

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare top failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pData->m_aMap);
	pSqlServer->BindString(2, pData->m_aName);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		return false;
	}

	if(!End)
	{
		// success
		pResult->m_MessageKind = EInstaSqlRequestType::ALL;

		int Rank = pSqlServer->GetInt(1);
		float Time = pSqlServer->GetFloat(2);
		// CEIL and FLOOR are not supported in SQLite
		int BetterThanPercent = std::floor(100.0f - (100.0f * pSqlServer->GetFloat(3)));
		str_time_float(Time, TIME_HOURS_CENTISECS, aBuf, sizeof(aBuf));

		if(str_comp_nocase(pData->m_aRequestingPlayer, pData->m_aName) == 0)
		{
			str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]),
				"%d. '%s' - flag time %s - better than %d%%",
				Rank, pData->m_aName, aBuf, BetterThanPercent);
		}
		else
		{
			str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]),
				"%d. '%s' - flag time %s - better than %d%% - requested by %s",
				Rank, pData->m_aName, aBuf, BetterThanPercent, pData->m_aRequestingPlayer);
		}
	}
	else
	{
		str_format(pResult->m_aaMessages[0], sizeof(pResult->m_aaMessages[0]),
			"'%s' is not ranked", pData->m_aName);
	}
	return true;
}

bool CSqlStats::SaveFastcapWorker(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	const auto *pData = dynamic_cast<const CSqlPlayerFastcapData *>(pGameData);

	// TODO: should we print something to the user on fastcap?
	// auto *pResult = dynamic_cast<CInstaSqlResult *>(pGameData->m_pResult.get());
	// auto *paMessages = pResult->m_aaMessages;

	char aBuf[1024];

	if(w == Write::NORMAL_SUCCEEDED)
	{
		str_format(aBuf, sizeof(aBuf),
			"DELETE FROM fastcaps_backup WHERE game_id=? AND name=? AND timestamp=%s",
			pSqlServer->InsertTimestampAsUtc());
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
			return false;
		}
		pSqlServer->BindString(1, pData->m_aGameUuid);
		pSqlServer->BindString(2, pData->m_aName);
		pSqlServer->BindString(3, pData->m_aTimestamp);
		pSqlServer->Print();
		int NumDeleted;
		if(!pSqlServer->ExecuteUpdate(&NumDeleted, pError, ErrorSize))
		{
			return false;
		}
		if(NumDeleted == 0)
		{
			log_warn("sql", "Fastcap rank got moved out of backup database, will show up as duplicate rank in MySQL");
		}
		return true;
	}
	if(w == Write::NORMAL_FAILED)
	{
		int NumUpdated;
		// move to non-tmp table succeeded. delete from backup again
		str_format(aBuf, sizeof(aBuf),
			"INSERT INTO fastcaps SELECT * FROM fastcaps_backup WHERE game_id=? AND name=? AND timestamp=%s",
			pSqlServer->InsertTimestampAsUtc());
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
			return false;
		}
		pSqlServer->BindString(1, pData->m_aGameUuid);
		pSqlServer->BindString(2, pData->m_aName);
		pSqlServer->BindString(3, pData->m_aTimestamp);
		pSqlServer->Print();
		if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
		{
			return false;
		}

		// move to non-tmp table succeeded. delete from backup again
		str_format(aBuf, sizeof(aBuf),
			"DELETE FROM fastcaps_backup WHERE game_id=? AND name=? AND timestamp=%s",
			pSqlServer->InsertTimestampAsUtc());
		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
			return false;
		}
		pSqlServer->BindString(1, pData->m_aGameUuid);
		pSqlServer->BindString(2, pData->m_aName);
		pSqlServer->BindString(3, pData->m_aTimestamp);
		pSqlServer->Print();
		if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
		{
			return false;
		}
		if(NumUpdated == 0)
		{
			log_warn("sql", "Fastcap rank got moved out of backup database, will show up as duplicate rank in MySQL");
		}
		return true;
	}

	// save fastcap. Can't fail, because no UNIQUE/PRIMARY KEY constrain is defined.
	str_format(aBuf, sizeof(aBuf),
		"%s INTO fastcaps%s("
		"       name, map, timestamp, time, server, "
		"       gametype, "
		"       grenade, stat_track, "
		"       game_id) "
		"VALUES (?, ?, %s, %.2f, ?, "
		"       ?, " // gametype
		"       %s, %s, "
		"       ?)", // game_id
		pSqlServer->InsertIgnore(),
		w == Write::NORMAL ? "" : "_backup",
		pSqlServer->InsertTimestampAsUtc(), pData->m_Time,
		pData->m_Grenade ? pSqlServer->True() : pSqlServer->False(),
		pData->m_StatTrack ? pSqlServer->True() : pSqlServer->False());
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pData->m_aName);
	pSqlServer->BindString(2, pData->m_aMap);
	pSqlServer->BindString(3, pData->m_aTimestamp);
	pSqlServer->BindString(4, g_Config.m_SvSqlServerName);
	pSqlServer->BindString(5, pData->m_aGametype);
	pSqlServer->BindString(6, pData->m_aGameUuid);
	pSqlServer->Print();
	int NumInserted;
	return pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize);
}

bool CSqlStats::SaveRoundStatsThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w != Write::NORMAL)
		return true;

	const CSqlSaveRoundStatsData *pData = dynamic_cast<const CSqlSaveRoundStatsData *>(pGameData);
	if(pData->m_DebugStats > 1)
	{
		dbg_msg("sql-thread", "writing stats of player '%s'", pData->m_aName);
		dbg_msg("sql-thread", "extra columns %p", pData->m_pExtraColumns);
	}

	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"SELECT"
		" name, points, kills, deaths, spree,"
		" win_points, wins, losses, shots_fired, shots_hit %s "
		"FROM %s "
		"WHERE name = ?;",
		!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->SelectColumns(),
		pData->m_aTable);
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pData->m_aName);
	pSqlServer->Print();

	if(pData->m_DebugStats > 1)
		dbg_msg("sql-thread", "select query: %s", aBuf);

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed");
		return false;
	}
	if(End)
	{
		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "inserting new record ...");
		// the _backup table is not used yet
		// because we guard the write mode at the top
		// to avoid complexity for now
		// but at some point we could write stats to a fallback database
		// and then merge them later
		str_format(
			aBuf,
			sizeof(aBuf),
			"%s INTO %s%s(" // INSERT INTO
			" name,"
			" points, kills, deaths, spree,"
			" win_points, wins, losses,"
			" shots_fired, shots_hit  %s"
			") VALUES ("
			" ?,"
			" ?, ?, ?, ?,"
			" ?, ?, ?,"
			" ?, ? %s"
			");",
			pSqlServer->InsertIgnore(),
			pData->m_aTable,
			w == Write::NORMAL ? "" : "_backup",
			!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->InsertColumns(),
			!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->InsertValues());

		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			dbg_msg("sql-thread", "prepare insert failed query=%s", aBuf);
			return false;
		}

		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "inserted query: %s", aBuf);

		int Offset = 1;
		pSqlServer->BindString(Offset++, pData->m_aName);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Points);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Kills);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Deaths);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_BestSpree);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_WinPoints);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Wins);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_Losses);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_ShotsFired);
		pSqlServer->BindInt(Offset++, pData->m_Stats.m_ShotsHit);

		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "pre extra offset %d", Offset);

		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->InsertBindings(&Offset, pSqlServer, &pData->m_Stats);

		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "final offset %d", Offset);

		pSqlServer->Print();

		int NumInserted;
		if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		{
			return false;
		}
	}
	else
	{
		if(pData->m_DebugStats > 1)
			dbg_msg("sql-thread", "updating existing record ...");

		CSqlStatsPlayer MergeStats;
		// 1 is name that we don't really need for now
		int Offset = 2;
		MergeStats.m_Points = pSqlServer->GetInt(Offset++);
		MergeStats.m_Kills = pSqlServer->GetInt(Offset++);
		MergeStats.m_Deaths = pSqlServer->GetInt(Offset++);
		MergeStats.m_BestSpree = pSqlServer->GetInt(Offset++);
		MergeStats.m_WinPoints = pSqlServer->GetInt(Offset++);
		MergeStats.m_Wins = pSqlServer->GetInt(Offset++);
		MergeStats.m_Losses = pSqlServer->GetInt(Offset++);
		MergeStats.m_ShotsFired = pSqlServer->GetInt(Offset++);
		MergeStats.m_ShotsHit = pSqlServer->GetInt(Offset++);

		if(pData->m_DebugStats > 1)
		{
			dbg_msg("sql-thread", "loaded stats:");
			MergeStats.Dump(pData->m_pExtraColumns, "sql-thread");
		}

		MergeStats.Merge(&pData->m_Stats);
		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->ReadAndMergeStats(&Offset, pSqlServer, &MergeStats, &pData->m_Stats);

		if(pData->m_DebugStats > 1)
		{
			dbg_msg("sql-thread", "merged stats:");
			MergeStats.Dump(pData->m_pExtraColumns, "sql-thread");
		}

		str_format(
			aBuf,
			sizeof(aBuf),
			"UPDATE %s%s "
			"SET"
			" points = ?, kills = ?, deaths = ?, spree = ?,"
			" win_points = ?, wins = ?, losses = ?,"
			" shots_fired = ?, shots_hit = ? %s"
			"WHERE name = ?;",
			pData->m_aTable,
			w == Write::NORMAL ? "" : "_backup",
			!pData->m_pExtraColumns ? "" : pData->m_pExtraColumns->UpdateColumns());

		if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
		{
			log_error("sql-thread", "prepare update failed query=%s", aBuf);
			return false;
		}

		Offset = 1;
		pSqlServer->BindInt(Offset++, MergeStats.m_Points);
		pSqlServer->BindInt(Offset++, MergeStats.m_Kills);
		pSqlServer->BindInt(Offset++, MergeStats.m_Deaths);
		pSqlServer->BindInt(Offset++, MergeStats.m_BestSpree);
		pSqlServer->BindInt(Offset++, MergeStats.m_WinPoints);
		pSqlServer->BindInt(Offset++, MergeStats.m_Wins);
		pSqlServer->BindInt(Offset++, MergeStats.m_Losses);
		pSqlServer->BindInt(Offset++, MergeStats.m_ShotsFired);
		pSqlServer->BindInt(Offset++, MergeStats.m_ShotsHit);

		if(pData->m_pExtraColumns)
			pData->m_pExtraColumns->UpdateBindings(&Offset, pSqlServer, &MergeStats);

		pSqlServer->BindString(Offset++, pData->m_aName);

		pSqlServer->Print();

		int NumUpdated;
		if(!pSqlServer->ExecuteUpdate(&NumUpdated, pError, ErrorSize))
		{
			return false;
		}

		if(NumUpdated == 0 && pData->m_Stats.HasValues())
		{
			log_error("sql-thread", "failed to save stats for player '%s'", pData->m_aName);
			log_error("sql-thread", "update failed no rows changed but got the following stats:");
			pData->m_Stats.Dump(pData->m_pExtraColumns, "sql-thread");
			return false;
		}
		else if(NumUpdated > 1)
		{
			log_error("sql-thread", "affected %d rows when trying to update stats of one player!", NumUpdated);
			dbg_assert(false, "FATAL ERROR: your database is probably corrupted! Time to restore the backup.");
			return false;
		}
	}
	return true;
}

void CSqlStats::CreateTable(const char *pName)
{
	auto Tmp = std::make_unique<CSqlCreateTableRequest>();
	str_copy(Tmp->m_aName, pName);
	Tmp->m_aColumns[0] = '\0';
	if(m_pExtraColumns)
		str_copy(Tmp->m_aColumns, m_pExtraColumns->CreateTable());
	m_pPool->ExecuteWrite(CreateTableThread, std::move(Tmp), "create table");
}

void CSqlStats::CreateFastcapTable()
{
	auto Tmp = std::make_unique<CSqlCreateTableRequest>();
	Tmp->m_aName[0] = '\0';
	Tmp->m_aColumns[0] = '\0';
	Tmp->m_aColumns[0] = '\0';
	m_pPool->ExecuteWrite(CreateFastcapTableThread, std::move(Tmp), "create fastcap table");
}

bool CSqlStats::CreateTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w == Write::NORMAL_FAILED)
	{
		if(!MysqlAvailable())
		{
			dbg_msg("sql-thread", "failed to create table! Make sure to compile with MySQL support if you want to use stats");
			return false;
		}

		dbg_assert(false, "CreateTableThread failed to write");
		return false;
	}
	const CSqlCreateTableRequest *pData = dynamic_cast<const CSqlCreateTableRequest *>(pGameData);

	// autoincrement not recommended by sqlite3
	// also its hard to be portable accross mysql and sqlite3
	// ddnet also uses any kind of unicode playername in the points update query

	char aBuf[4096];
	str_format(aBuf, sizeof(aBuf),
		"CREATE TABLE IF NOT EXISTS %s%s("
		"name        VARCHAR(%d)   COLLATE %s  NOT NULL,"
		"first_seen  TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP, "
		"points      INTEGER       DEFAULT 0,"
		"kills       INTEGER       DEFAULT 0,"
		"deaths      INTEGER       DEFAULT 0,"
		"spree       INTEGER       DEFAULT 0,"
		"win_points  INTEGER       DEFAULT 0,"
		"wins        INTEGER       DEFAULT 0,"
		"losses      INTEGER       DEFAULT 0,"
		"shots_fired INTEGER       DEFAULT 0,"
		"shots_hit   INTEGER       DEFAULT 0,"
		"%s"
		"PRIMARY KEY (name)"
		");",
		pData->m_aName,
		w == Write::NORMAL ? "" : "_backup",
		MAX_NAME_LENGTH_SQL,
		pSqlServer->BinaryCollate(),
		pData->m_aColumns);

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return false;
	}
	pSqlServer->Print();
	int NumInserted;
	if(!pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize))
		return false;

	// apply missing migrations
	// this is for seamless backwards compatibility
	// upgrade database schema automatically

	bool (*pfnAddInt)(IDbConnection *, const char *, const char *, char *, int) = nullptr;

	if(DetectBackend(pSqlServer) == ESqlBackend::SQLITE3)
		pfnAddInt = AddColumnIntDefault0Sqlite3;
	else if(DetectBackend(pSqlServer) == ESqlBackend::MYSQL)
		pfnAddInt = AddColumnIntDefault0Mysql;

	if(pfnAddInt)
		pfnAddInt(pSqlServer, pData->m_aName, "win_points", pError, ErrorSize);

	return true;
}

bool CSqlStats::AddIntColumn(IDbConnection *pSqlServer, const char *pTableName, const char *pColumnName, int Default, char *pError, int ErrorSize)
{
	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"ALTER TABLE %s ADD COLUMN %s INTEGER DEFAULT %d;", pTableName, pColumnName, Default);

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return false;
	}
	pSqlServer->Print();
	int NumInserted;
	return pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize);
}

bool CSqlStats::AddColumnIntDefault0Sqlite3(IDbConnection *pSqlServer, const char *pTableName, const char *pColumnName, char *pError, int ErrorSize)
{
	char aBuf[4096];
	str_copy(
		aBuf,
		"SELECT COUNT() FROM pragma_table_info(?) WHERE name = ?;");
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pTableName);
	pSqlServer->BindString(2, pColumnName);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed query: %s", aBuf);
		return false;
	}

	if(End)
	{
		// we expect 0 or 1 but never nothing
		dbg_msg("sql-thread", "something went wrong failed query: %s", aBuf);
		return false;
	}
	if(pSqlServer->GetInt(1) == 0)
	{
		log_info("sql-thread", "adding missing sqlite3 column '%s' to '%s'", pColumnName, pTableName);
		return !AddIntColumn(pSqlServer, pTableName, pColumnName, 0, pError, ErrorSize);
	}
	return true;
}

bool CSqlStats::AddColumnIntDefault0Mysql(IDbConnection *pSqlServer, const char *pTableName, const char *pColumnName, char *pError, int ErrorSize)
{
	char aBuf[4096];
	str_format(
		aBuf,
		sizeof(aBuf),
		"show columns from %s where Field = ?;",
		pTableName);
	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "prepare failed query: %s", aBuf);
		return false;
	}
	pSqlServer->BindString(1, pColumnName);
	pSqlServer->Print();

	bool End;
	if(!pSqlServer->Step(&End, pError, ErrorSize))
	{
		dbg_msg("sql-thread", "step failed query: %s", aBuf);
		return false;
	}
	if(!End)
		return true;

	log_info("sql-thread", "adding missing mysql column '%s' to '%s'", pColumnName, pTableName);
	return !AddIntColumn(pSqlServer, pTableName, pColumnName, 0, pError, ErrorSize);
}

bool CSqlStats::CreateFastcapTableThread(IDbConnection *pSqlServer, const ISqlData *pGameData, Write w, char *pError, int ErrorSize)
{
	if(w == Write::NORMAL_FAILED)
	{
		if(!MysqlAvailable())
		{
			dbg_msg("sql-thread", "failed to create fastcap table! Make sure to compile with MySQL support if you want to use stats");
			return false;
		}

		dbg_assert(false, "CreateFastcapTableThread failed to write");
		return false;
	}

	char aBuf[4096];
	str_format(aBuf, sizeof(aBuf),
		"CREATE TABLE IF NOT EXISTS fastcaps%s("
		" name        VARCHAR(%d)   COLLATE %s NOT NULL,"
		" map         VARCHAR(128)  COLLATE %s NOT NULL, "
		" timestamp   TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP, "
		" time        FLOAT         DEFAULT 0, "
		" server      CHAR(4), "
		" gametype    VARCHAR(128)  COLLATE %s NOT NULL,"
		" grenade     BOOL          DEFAULT FALSE,"
		" stat_track  BOOL          DEFAULT FALSE,"
		" game_id     VARCHAR(64), "
		"PRIMARY KEY (name, map, time, timestamp)"
		");",
		w == Write::NORMAL ? "" : "_backup",
		MAX_NAME_LENGTH_SQL,
		pSqlServer->BinaryCollate(),
		pSqlServer->BinaryCollate(),
		pSqlServer->BinaryCollate());

	if(!pSqlServer->PrepareStatement(aBuf, pError, ErrorSize))
	{
		return false;
	}
	pSqlServer->Print();
	int NumInserted;
	return pSqlServer->ExecuteUpdate(&NumInserted, pError, ErrorSize);
}
