#include "ctf.h"

#include <base/system.h>

#include <engine/server.h>
#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/entities/flag.h>

CGameControllerInstaBaseCTF::CGameControllerInstaBaseCTF(class CGameContext *pGameServer) :
	CGameControllerInstagib(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
}

CGameControllerInstaBaseCTF::~CGameControllerInstaBaseCTF() = default;

void CGameControllerInstaBaseCTF::Tick()
{
	CGameControllerBasePvp::Tick();

	FlagTick(); // ddnet-insta
}

void CGameControllerInstaBaseCTF::OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	CGameControllerInstagib::OnShowStatsAll(pStats, pRequestingPlayer, pRequestedName);

	char aBuf[512];

	str_format(aBuf, sizeof(aBuf), "~ Flag grabs: %d", pStats->m_FlagGrabs);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Flag captures: %d", pStats->m_FlagCaptures);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Flagger kills: %d", pStats->m_FlaggerKills);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerInstaBaseCTF::OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	char aBuf[512];

	str_format(aBuf, sizeof(aBuf), "~ Flag grabs: %d", pStats->m_FlagGrabs);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Flag captures: %d", pStats->m_FlagCaptures);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Flagger kills: %d", pStats->m_FlaggerKills);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerInstaBaseCTF::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBasePvp::OnCharacterSpawn(pChr);
}

int CGameControllerInstaBaseCTF::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerBasePvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
	int HadFlag = 0;

	// drop flags
	for(CFlag *pFlag : m_apFlags)
	{
		if(pFlag && pKiller && pKiller->GetCharacter() && pFlag->GetCarrier() == pKiller->GetCharacter())
			HadFlag |= 2;
		if(pFlag && pFlag->GetCarrier() == pVictim)
		{
			GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
			GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_DROP, -1);
			pFlag->Drop();
			// https://github.com/ddnet-insta/ddnet-insta/issues/156
			pFlag->m_pLastCarrier = nullptr;

			if(pKiller && pKiller->GetTeam() != pVictim->GetPlayer()->GetTeam())
			{
				pKiller->IncrementScore();
				if(IsStatTrack())
					pKiller->m_Stats.m_FlaggerKills++;
			}

			HadFlag |= 1;
		}
		if(pFlag && pFlag->GetCarrier() == pVictim)
			pFlag->SetCarrier(0);
	}

	return HadFlag;
}

bool CGameControllerInstaBaseCTF::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerInstagib::OnEntity(Index, x, y, Layer, Flags, Initial, Number);

	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
	int Team = -1;
	if(Index == ENTITY_FLAGSTAND_RED)
		Team = TEAM_RED;
	if(Index == ENTITY_FLAGSTAND_BLUE)
		Team = TEAM_BLUE;

	if(Team != -1 && g_Config.m_SvSwapFlags)
		Team = Team == TEAM_RED ? TEAM_BLUE : TEAM_RED;

	if(Team == -1 || m_apFlags[Team])
		return false;

	CFlag *pFlag = new CFlag(&GameServer()->m_World, Team);
	pFlag->m_StandPos = Pos;
	pFlag->m_Pos = Pos;
	m_apFlags[Team] = pFlag;
	GameServer()->m_World.InsertEntity(pFlag);
	return true;
}

bool CGameControllerInstaBaseCTF::CanBeMovedOnBalance(int ClientId)
{
	return GetCarriedFlag(GameServer()->m_apPlayers[ClientId]) == FLAG_NONE;
}
