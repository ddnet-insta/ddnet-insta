#include <base/system.h>
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include "ctf.h"

CGameControllerInstaBaseCTF::CGameControllerInstaBaseCTF(class CGameContext *pGameServer) :
	CGameControllerInstagib(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
}

CGameControllerInstaBaseCTF::~CGameControllerInstaBaseCTF() = default;

void CGameControllerInstaBaseCTF::Tick()
{
	CGameControllerPvp::Tick();

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

bool CGameControllerInstaBaseCTF::OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId)
{
	if(!g_Config.m_SvDropFlagOnVote)
		return false;

	if(pMsg->m_Vote != 1) // 1 is yes
		return false;

	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;

	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return false;

	DropFlag(pChr);

	// returning false here lets the vote go through
	// so pressing vote yes as flag carrier during a vote
	// will send an actual vote AND drop the flag
	return false;
}

bool CGameControllerInstaBaseCTF::OnSelfkill(int ClientId)
{
	if(!g_Config.m_SvDropFlagOnSelfkill)
		return false;

	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return false;

	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return false;

	return DropFlag(pChr);
}

bool CGameControllerInstaBaseCTF::DropFlag(class CCharacter *pChr)
{
	for(CFlag *pFlag : m_apFlags)
	{
		if(!pFlag)
			continue;
		if(pFlag->GetCarrier() != pChr)
			continue;

		GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
		GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_DROP, -1);

		vec2 Dir = vec2(5 * pChr->GetAimDir(), -5);
		pFlag->Drop(Dir);
		return true;
	}
	return false;
}

void CGameControllerInstaBaseCTF::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerPvp::OnCharacterSpawn(pChr);
}

int CGameControllerInstaBaseCTF::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerPvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
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

void CGameControllerInstaBaseCTF::OnFlagReturn(CFlag *pFlag)
{
	CGameControllerPvp::OnFlagReturn(pFlag);

	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", "flag_return");
	GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_RETURN, -1);
	GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
}

void CGameControllerInstaBaseCTF::OnFlagGrab(class CFlag *pFlag)
{
	if(!pFlag)
		return;
	if(!pFlag->IsAtStand())
		return;
	if(!pFlag->GetCarrier())
		return;

	CPlayer *pPlayer = pFlag->GetCarrier()->GetPlayer();
	if(IsStatTrack())
		pPlayer->m_Stats.m_FlagGrabs++;
}

void CGameControllerInstaBaseCTF::OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks)
{
	CGameControllerInstagib::OnFlagCapture(pFlag, Time, TimeTicks);

	if(!pFlag)
		return;
	if(!pFlag->m_pCarrier)
		return;

	CPlayer *pPlayer = pFlag->m_pCarrier->GetPlayer();
	if(IsStatTrack())
		pPlayer->m_Stats.m_FlagCaptures++;
}

void CGameControllerInstaBaseCTF::FlagTick()
{
	if(GameServer()->m_World.m_ResetRequested || GameServer()->m_World.m_Paused)
		return;

	for(int FlagColor = 0; FlagColor < 2; FlagColor++)
	{
		CFlag *pFlag = m_apFlags[FlagColor];

		if(!pFlag)
			continue;

		//
		if(pFlag->GetCarrier())
		{
			// forbid holding flags in ddrace teams
			if(GameServer()->GetDDRaceTeam(pFlag->GetCarrier()->GetPlayer()->GetCid()))
			{
				GameServer()->CreateSoundGlobal(SOUND_CTF_DROP);
				GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_DROP, -1);
				pFlag->Drop();
				continue;
			}

			if(m_apFlags[FlagColor ^ 1] && m_apFlags[FlagColor ^ 1]->IsAtStand())
			{
				if(distance(pFlag->GetPos(), m_apFlags[FlagColor ^ 1]->GetPos()) < CFlag::ms_PhysSize + CCharacterCore::PhysicalSize())
				{
					// CAPTURE! \o/
					AddTeamscore(FlagColor ^ 1, 100);
					pFlag->GetCarrier()->GetPlayer()->AddScore(5);
					float Diff = Server()->Tick() - pFlag->GetGrabTick();

					char aBuf[64];
					str_format(aBuf, sizeof(aBuf), "flag_capture player='%d:%s' team=%d time=%.2f",
						pFlag->GetCarrier()->GetPlayer()->GetCid(),
						Server()->ClientName(pFlag->GetCarrier()->GetPlayer()->GetCid()),
						pFlag->GetCarrier()->GetPlayer()->GetTeam(),
						Diff / (float)Server()->TickSpeed());
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

					float CaptureTime = Diff / (float)Server()->TickSpeed();
					if(CaptureTime <= 60)
						str_format(aBuf,
							sizeof(aBuf),
							"The %s flag was captured by '%s' (%d.%s%d seconds)", FlagColor ? "blue" : "red",
							Server()->ClientName(pFlag->GetCarrier()->GetPlayer()->GetCid()), (int)CaptureTime % 60, ((int)(CaptureTime * 100) % 100) < 10 ? "0" : "", (int)(CaptureTime * 100) % 100);
					else
						str_format(
							aBuf,
							sizeof(aBuf),
							"The %s flag was captured by '%s'", FlagColor ? "blue" : "red",
							Server()->ClientName(pFlag->GetCarrier()->GetPlayer()->GetCid()));
					for(auto &pPlayer : GameServer()->m_apPlayers)
					{
						if(!pPlayer)
							continue;
						if(Server()->IsSixup(pPlayer->GetCid()))
							continue;

						GameServer()->SendChatTarget(pPlayer->GetCid(), aBuf);
					}
					GameServer()->m_pController->OnFlagCapture(pFlag, Diff, Server()->Tick() - pFlag->GetGrabTick());
					GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_CAPTURE, FlagColor, pFlag->GetCarrier()->GetPlayer()->GetCid(), Diff, -1);
					GameServer()->CreateSoundGlobal(SOUND_CTF_CAPTURE);

					for(CFlag *pF : m_apFlags)
						pF->Reset();
					// do a win check(capture could trigger win condition)
					if(DoWincheckRound())
						return;
				}
			}
		}
		else
		{
			CCharacter *apCloseCCharacters[MAX_CLIENTS];
			int Num = GameServer()->m_World.FindEntities(pFlag->GetPos(), CFlag::ms_PhysSize, (CEntity **)apCloseCCharacters, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; i++)
			{
				if(!apCloseCCharacters[i]->IsAlive() || apCloseCCharacters[i]->GetPlayer()->GetTeam() == TEAM_SPECTATORS || GameServer()->Collision()->IntersectLine(pFlag->GetPos(), apCloseCCharacters[i]->GetPos(), NULL, NULL))
					continue;

				// only allow flag grabs in team 0
				if(GameServer()->GetDDRaceTeam(apCloseCCharacters[i]->GetPlayer()->GetCid()))
					continue;

				// cooldown for recollect after dropping the flag
				if(pFlag->m_pLastCarrier == apCloseCCharacters[i] && (pFlag->m_DropTick + Server()->TickSpeed()) > Server()->Tick())
					continue;

				if(apCloseCCharacters[i]->GetPlayer()->GetTeam() == pFlag->GetTeam())
				{
					// return the flag
					if(!pFlag->IsAtStand())
					{
						CCharacter *pChr = apCloseCCharacters[i];
						pChr->GetPlayer()->IncrementScore();

						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "flag_return player='%d:%s' team=%d",
							pChr->GetPlayer()->GetCid(),
							Server()->ClientName(pChr->GetPlayer()->GetCid()),
							pChr->GetPlayer()->GetTeam());
						GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
						GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_RETURN, -1);
						GameServer()->CreateSoundGlobal(SOUND_CTF_RETURN);
						pFlag->Reset();
					}
				}
				else
				{
					// take the flag
					if(pFlag->IsAtStand())
						AddTeamscore(FlagColor ^ 1, 1);

					pFlag->Grab(apCloseCCharacters[i]);

					pFlag->GetCarrier()->GetPlayer()->IncrementScore();

					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "flag_grab player='%d:%s' team=%d",
						pFlag->GetCarrier()->GetPlayer()->GetCid(),
						Server()->ClientName(pFlag->GetCarrier()->GetPlayer()->GetCid()),
						pFlag->GetCarrier()->GetPlayer()->GetTeam());
					GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
					GameServer()->SendGameMsg(protocol7::GAMEMSG_CTF_GRAB, FlagColor, -1);
					break;
				}
			}
		}
	}
	DoWincheckRound();
}

void CGameControllerInstaBaseCTF::Snap(int SnappingClient)
{
	CGameControllerPvp::Snap(SnappingClient);

	int FlagCarrierRed = FLAG_MISSING;
	if(m_apFlags[TEAM_RED])
	{
		if(m_apFlags[TEAM_RED]->m_AtStand)
			FlagCarrierRed = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_RED]->GetCarrier() && m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer())
			FlagCarrierRed = m_apFlags[TEAM_RED]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierRed = FLAG_TAKEN;
	}

	int FlagCarrierBlue = FLAG_MISSING;
	if(m_apFlags[TEAM_BLUE])
	{
		if(m_apFlags[TEAM_BLUE]->m_AtStand)
			FlagCarrierBlue = FLAG_ATSTAND;
		else if(m_apFlags[TEAM_BLUE]->GetCarrier() && m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer())
			FlagCarrierBlue = m_apFlags[TEAM_BLUE]->GetCarrier()->GetPlayer()->GetCid();
		else
			FlagCarrierBlue = FLAG_TAKEN;
	}

	if(Server()->IsSixup(SnappingClient))
	{
		protocol7::CNetObj_GameDataFlag *pGameDataObj = Server()->SnapNewItem<protocol7::CNetObj_GameDataFlag>(0);
		if(!pGameDataObj)
			return;

		pGameDataObj->m_FlagCarrierRed = FlagCarrierRed;
		pGameDataObj->m_FlagCarrierBlue = FlagCarrierBlue;
	}
	else
	{
		CNetObj_GameData *pGameDataObj = Server()->SnapNewItem<CNetObj_GameData>(0);
		if(!pGameDataObj)
			return;

		pGameDataObj->m_FlagCarrierRed = FlagCarrierRed;
		pGameDataObj->m_FlagCarrierBlue = FlagCarrierBlue;

		pGameDataObj->m_TeamscoreRed = m_aTeamscore[TEAM_RED];
		pGameDataObj->m_TeamscoreBlue = m_aTeamscore[TEAM_BLUE];
	}
}

bool CGameControllerInstaBaseCTF::DoWincheckRound()
{
	if(IsWarmup())
		return false;

	CGameControllerPvp::DoWincheckRound();

	// check score win condition
	if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
		(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
	{
		if(m_SuddenDeath)
		{
			if(m_aTeamscore[TEAM_RED] / 100 != m_aTeamscore[TEAM_BLUE] / 100)
			{
				EndRound();
				return true;
			}
		}
		else
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE])
			{
				EndRound();
				return true;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	return false;
}
