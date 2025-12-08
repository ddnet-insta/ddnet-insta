#include "round_stats_player.h"

#include <generated/protocol.h>

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/strhelpers.h>
#include <game/server/player.h>

void IGameController::GetRoundEndStatsStrCsv(char *pBuf, size_t Size)
{
	if(IsTeamPlay())
		GetRoundEndStatsStrCsvTeamPlay(pBuf, Size);
	else
		GetRoundEndStatsStrCsvNoTeamPlay(pBuf, Size);
}

void IGameController::GetRoundEndStatsStrCsvTeamPlay(char *pBuf, size_t Size)
{
	pBuf[0] = '\0';
	char aBuf[512];

	int ScoreRed = m_aTeamscore[TEAM_RED];
	int ScoreBlue = m_aTeamscore[TEAM_BLUE];

	// csv header
	str_append(pBuf, "red_name, red_score, blue_name, blue_score\n", Size);

	// insert blue and red as if they were players called blue and red
	// as the first entry
	str_format(aBuf, sizeof(aBuf), "red, %d, blue, %d\n", ScoreRed, ScoreBlue);
	str_append(pBuf, aBuf, Size);

	CStatsPlayer aStatsPlayerRed[MAX_CLIENTS];
	CStatsPlayer aStatsPlayerBlue[MAX_CLIENTS];

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if(!pPlayer)
			continue;
		if(pPlayer->GetTeam() < TEAM_RED)
			continue;
		if(pPlayer->GetTeam() > TEAM_BLUE)
			continue;

		CStatsPlayer *pStatsPlayer = pPlayer->GetTeam() == TEAM_RED ? &aStatsPlayerRed[i] : &aStatsPlayerBlue[i];
		pStatsPlayer->m_Active = true;
		pStatsPlayer->m_Score = pPlayer->m_Score.value_or(0);
		pStatsPlayer->m_pName = Server()->ClientName(pPlayer->GetCid());
	}

	std::stable_sort(aStatsPlayerRed, aStatsPlayerRed + MAX_CLIENTS,
		[](const CStatsPlayer &p1, const CStatsPlayer &p2) -> bool {
			return p1.m_Score > p2.m_Score;
		});
	std::stable_sort(aStatsPlayerBlue, aStatsPlayerBlue + MAX_CLIENTS,
		[](const CStatsPlayer &p1, const CStatsPlayer &p2) -> bool {
			return p1.m_Score > p2.m_Score;
		});

	int RedIndex = 0;
	int BlueIndex = 0;
	const CStatsPlayer *pRed = &aStatsPlayerRed[RedIndex];
	const CStatsPlayer *pBlue = &aStatsPlayerBlue[BlueIndex];
	while(pRed || pBlue)
	{
		// mom: "we have pop() at home"
		// the pop() at home:
		while(pRed && !pRed->m_Active)
		{
			if(++RedIndex >= MAX_CLIENTS)
			{
				pRed = nullptr;
				break;
			};
			pRed = &aStatsPlayerRed[RedIndex];
		}
		while(pBlue && !pBlue->m_Active)
		{
			if(++BlueIndex >= MAX_CLIENTS)
			{
				pBlue = nullptr;
				break;
			};
			pBlue = &aStatsPlayerBlue[BlueIndex];
		}
		if(!pBlue && !pRed)
			break;

		// dbg_msg("debug", "RedIndex=%d BlueIndex=%d pRed=%p pBlue=%p", RedIndex, BlueIndex, pRed, pBlue);

		char aEscapedNameRed[512];
		char aEscapedNameBlue[512];
		str_escape_csv(aEscapedNameRed, sizeof(aEscapedNameRed), pRed->m_pName);
		str_escape_csv(aEscapedNameBlue, sizeof(aEscapedNameBlue), pBlue->m_pName);

		str_format(
			aBuf,
			sizeof(aBuf),
			"%s, %d, %s, %d\n",
			pRed ? aEscapedNameRed : "",
			pRed ? pRed->m_Score : 0,
			pBlue ? aEscapedNameBlue : "",
			pBlue ? pBlue->m_Score : 0);
		str_append(pBuf, aBuf, Size);

		if(++RedIndex >= MAX_CLIENTS)
		{
			pRed = nullptr;
		}
		else
		{
			pRed = &aStatsPlayerRed[RedIndex];
		}
		if(++BlueIndex >= MAX_CLIENTS)
		{
			pBlue = nullptr;
		}
		else
		{
			pBlue = &aStatsPlayerBlue[BlueIndex];
		}
	}
}

void IGameController::GetRoundEndStatsStrCsvNoTeamPlay(char *pBuf, size_t Size)
{
	pBuf[0] = '\0';
	char aBuf[512];

	// csv header
	str_append(pBuf, "score, name", Size);
	if(WinType() == WIN_BY_SURVIVAL)
		str_append(pBuf, ", alive", Size);
	str_append(pBuf, "\n", Size);

	CStatsPlayer aStatsPlayers[MAX_CLIENTS];

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		if(!pPlayer)
			continue;
		if(!IsPlaying(pPlayer))
			continue;

		CStatsPlayer *pStatsPlayer = &aStatsPlayers[i];
		pStatsPlayer->m_Active = true;
		pStatsPlayer->m_IsDead = pPlayer->m_IsDead;
		pStatsPlayer->m_Score = pPlayer->m_Score.value_or(0);
		pStatsPlayer->m_pName = Server()->ClientName(pPlayer->GetCid());
	}

	std::stable_sort(aStatsPlayers, aStatsPlayers + MAX_CLIENTS,
		[](const CStatsPlayer &p1, const CStatsPlayer &p2) -> bool {
			return p1.m_Score > p2.m_Score;
		});

	for(CStatsPlayer Player : aStatsPlayers)
	{
		if(!Player.m_Active)
			continue;

		char aEscapedName[512];
		str_escape_csv(aEscapedName, sizeof(aEscapedName), Player.m_pName);

		str_format(
			aBuf,
			sizeof(aBuf),
			"%d, %s",
			Player.m_Score,
			aEscapedName);
		str_append(pBuf, aBuf, Size);
		if(WinType() == WIN_BY_SURVIVAL)
			str_append(pBuf, Player.m_IsDead ? ", no" : ", yes", Size);
		str_append(pBuf, "\n", Size);
	}
}
