#include "zcatch.h"

#include <base/log.h>

#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>
#include <generated/protocol7.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include <insta/server/dead_spec_controller.h>
#include <insta/server/gamemodes/instagib/base_instagib.h>

CGameControllerZcatch::CGameControllerZcatch(class CGameContext *pGameServer) :
	CGameControllerInstagib(pGameServer)
{
	m_GameFlags = 0;
	m_pGameType = "zCatch";
	m_WinType = WIN_BY_SURVIVAL;
	m_DefaultWeapon = GetDefaultWeaponBasedOnSpawnWeapons();
	m_pDeadSpecController = new CDeadSpecController(this, pGameServer);
	m_pStatsTable = "";
	if(m_SpawnWeapons == ESpawnWeapons::SPAWN_WEAPON_GRENADE)
		m_pStatsTable = "zcatch_grenade";
	else if(m_SpawnWeapons == ESpawnWeapons::SPAWN_WEAPON_LASER)
		m_pStatsTable = "zcatch_laser";
	if(m_pStatsTable[0])
	{
		m_pExtraColumns = new CZcatchColumns();
		Db()->Stats()->SetExtraColumns(m_pExtraColumns);
		Db()->Stats()->CreateTable(m_pStatsTable);
	}
}

void CGameControllerZcatch::OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	CGameControllerInstagib::OnShowStatsAll(pStats, pRequestingPlayer, pRequestedName);

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "~ Win points: %d", pStats->m_WinPoints);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Seconds in game: %" PRId64, pStats->m_TicksAlive / Server()->TickSpeed());
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Seconds caught: %" PRId64, pStats->m_TicksDead / Server()->TickSpeed());
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerZcatch::OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	CGameControllerInstagib::OnShowRoundStats(pStats, pRequestingPlayer, pRequestedName);

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "~ Seconds in game: %" PRId64, pStats->m_TicksAlive / Server()->TickSpeed());
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Seconds caught: %" PRId64, pStats->m_TicksDead / Server()->TickSpeed());
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Kills that give points on win: %d", pRequestingPlayer->m_KillsThatCount);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);

	str_format(aBuf, sizeof(aBuf), "~ Kills that can be released: %" PRIzu, pRequestingPlayer->m_vVictimIds.size());
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

CGameControllerZcatch::ECatchGameState CGameControllerZcatch::CatchGameState() const
{
	if(g_Config.m_SvReleaseGame)
		return ECatchGameState::RELEASE_GAME;
	return m_CatchGameState;
}

bool CGameControllerZcatch::IsCatchGameRunning() const
{
	return CatchGameState() == ECatchGameState::RUNNING;
}

void CGameControllerZcatch::SetCatchGameState(ECatchGameState State)
{
	if(g_Config.m_SvReleaseGame)
	{
		m_CatchGameState = ECatchGameState::RELEASE_GAME;
		return;
	}
	bool Changed = State != m_CatchGameState;
	if(Changed)
	{
		if(State == ECatchGameState::RUNNING)
		{
			KillAllPlayers();
			StartZcatchRound();
		}
		ReleaseAllPlayers();
	}

	m_CatchGameState = State;

	// set and reset spawn timer for all
	// to track their in game ticks
	if(Changed)
	{
		for(CPlayer *pPlayer : GameServer()->m_apPlayers)
		{
			if(!pPlayer)
				continue;

			if(!IsCatchGameRunning())
				UpdateCatchTicks(pPlayer, ECatchUpdate::ROUND_END);
			else
				UpdateCatchTicks(pPlayer, ECatchUpdate::CONNECT);
		}
	}
}

bool CGameControllerZcatch::IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage)
{
	if(pMessage && SizeOfMessage)
		pMessage[0] = '\0';

	// you can only win as last alive player
	// used for disconnect IsWinner check
	if(NumNonDeadActivePlayers() > 1)
		return false;
	if(pPlayer->GetTeam() == TEAM_SPECTATORS)
		return false;
	if(pPlayer->m_IsDead)
		return false;
	// you can never win with 0 kills
	// this should cover edge cases where one spawns into the world
	// where everyone else is currently in a death screen
	if(!pPlayer->m_Spree)
		return false;
	// you can not win a round with less than 4 kills
	// if players leave after the game started
	// before they got killed by the leading player
	// the leading player has to wait for new players to join
	if(pPlayer->m_KillsThatCount < MIN_ZCATCH_KILLS)
		return false;
	// there are no winners in release games even if the round ends
	if(!IsCatchGameRunning())
		return false;

	if(pMessage)
		str_copy(pMessage, "+1 win was saved on your name (see /rank_wins).", SizeOfMessage);

	return true;
}

bool CGameControllerZcatch::IsLoser(const CPlayer *pPlayer)
{
	// you can only win running games
	// so you can also only lose running games
	if(IsCatchGameRunning())
		return false;

	// rage quit as dead player is counted as a loss
	// qutting mid game while being alive is not
	return pPlayer->m_IsDead;
}

int CGameControllerZcatch::WinPointsForWin(const CPlayer *pPlayer)
{
	int Kills = pPlayer->m_KillsThatCount;
	int Points = 0;
	// 0 points should not be possible on round end
	// because you can only end it with 4 or more kills
	if(Kills < 4) // 1-3
		Points = 0;
	else if(Kills <= 6) // 5-6
		Points = 1;
	else if(Kills <= 8) // 7-8
		Points = 2;
	else if(Kills == 9) // 9
		Points = 3;
	else if(Kills == 10) // 10
		Points = 5;
	else if(Kills == 11) // 11
		Points = 7;
	else if(Kills == 12) // 12
		Points = 9;
	else if(Kills == 13) // 13
		Points = 11;
	else if(Kills == 14) // 14
		Points = 12;
	else if(Kills == 15) // 15
		Points = 14;
	else // 16+
		Points = 16;

	log_info(
		"zcatch",
		"player '%s' earned %d win_points for winning with %d kills",
		Server()->ClientName(pPlayer->GetCid()),
		Points,
		Kills);
	return Points;
}

void CGameControllerZcatch::StartZcatchRound()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_GotRespawnInfo = false;
		pPlayer->m_vVictimIds.clear(); // TODO: these victims have to be released!
		pPlayer->m_KillerId = -1;

		// resets the winners color
		pPlayer->m_Spree = 0; // TODO: this is nasty it breaks sprees across rounds
		//                             also resetting m_Spree should check first
		//                             if it was a new spree high score and set
		//                             m_BestSpree
		pPlayer->m_UntrackedSpree = 0;
	}
}

void CGameControllerZcatch::OnRoundStart()
{
	CGameControllerInstagib::OnRoundStart();

	int ActivePlayers = NumActivePlayers();
	if(ActivePlayers < MIN_ZCATCH_PLAYERS && CatchGameState() != ECatchGameState::RELEASE_GAME)
	{
		SendChatTarget(-1, "Not enough players to start a round");
		SetCatchGameState(ECatchGameState::WAITING_FOR_PLAYERS);
	}
	StartZcatchRound();

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		UpdateCatchTicks(pPlayer, ECatchUpdate::CONNECT);
	}
}

void CGameControllerZcatch::OnRoundEnd()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		UpdateCatchTicks(pPlayer, ECatchUpdate::ROUND_END);
	}

	CGameControllerInstagib::OnRoundEnd();
}

CGameControllerZcatch::~CGameControllerZcatch() = default;

void CGameControllerZcatch::Tick()
{
	CGameControllerInstagib::Tick();
}

void CGameControllerZcatch::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerInstagib::OnCharacterSpawn(pChr);

	SetSpawnWeapons(pChr);

	ResetKillsThatCount(pChr->GetPlayer()); // just to be sure
}

void CGameControllerZcatch::ReleasePlayer(class CPlayer *pPlayer, const char *pMsg)
{
	GameServer()->SendChatTarget(pPlayer->GetCid(), pMsg);
	UpdateCatchTicks(pPlayer, ECatchUpdate::RELEASE);

	m_pDeadSpecController->RespawnPlayer(pPlayer);
}

bool CGameControllerZcatch::DoSomethingElseInsteadOfSelfkill(CPlayer *pPlayer)
{
	if(CGameControllerInstagib::DoSomethingElseInsteadOfSelfkill(pPlayer))
		return true;

	int ClientId = pPlayer->GetCid();
	if(pPlayer->m_vVictimIds.empty())
		return false;

	CPlayer *pVictim = nullptr;
	while(!pVictim)
	{
		if(pPlayer->m_vVictimIds.empty())
			return false;

		int ReleaseId = pPlayer->m_vVictimIds.back();
		pPlayer->m_vVictimIds.pop_back();

		pVictim = GameServer()->m_apPlayers[ReleaseId];
	}
	if(!pVictim)
	{
		// TODO: should this be an assert?? we should never have a victim id
		//       stored of a player that already left
		log_error("zcatch", "cid=%d tried to release player that does not exist", ClientId);
		return false;
	}

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "You were released by '%s'", Server()->ClientName(pPlayer->GetCid()));
	ReleasePlayer(pVictim, aBuf);

	str_format(aBuf, sizeof(aBuf), "You released '%s' (%" PRIzu " players left)", Server()->ClientName(pVictim->GetCid()), pPlayer->m_vVictimIds.size());
	SendChatTarget(ClientId, aBuf);

	// the kill count should never go negative
	// that could happen if you made 1 kill and then 3 new spectators joined
	// if all of them get released manually we should arrive at 0 kills
	// not at -2
	//
	// https://github.com/ddnet-insta/ddnet-insta/issues/225
	AddToKillsThatCount(pPlayer, -1);
	if(pPlayer->m_KillsThatCount <= 0)
	{
		ResetKillsThatCount(pPlayer);
		for(int VictimId : pPlayer->m_vVictimIds)
		{
			pVictim = GameServer()->m_apPlayers[VictimId];
			if(!pVictim)
				continue;

			str_format(aBuf, sizeof(aBuf), "You were released by '%s'", Server()->ClientName(pPlayer->GetCid()));
			ReleasePlayer(pVictim, aBuf);
		}
		if(pPlayer->m_vVictimIds.empty())
			str_copy(aBuf, "You released all players. The next selfkill will kill you!");
		else
			str_format(aBuf, sizeof(aBuf), "You released %" PRIzu " remaining spectators because your kill count reached 0.", pPlayer->m_vVictimIds.size());
		SendChatTarget(ClientId, aBuf);
		pPlayer->m_vVictimIds.clear();
	}
	return true;
}

void CGameControllerZcatch::KillPlayer(class CPlayer *pVictim, class CPlayer *pKiller, bool KillCounts)
{
	if(!pKiller)
		return;
	if(!pKiller->GetCharacter())
		return;
	if(pKiller->GetTeam() == TEAM_SPECTATORS)
		return;
	if(pKiller->m_IsDead)
	{
		log_warn(
			"zcatch",
			"warning '%s' was killed by the dead (but not spec) player '%s'",
			Server()->ClientName(pVictim->GetCid()),
			Server()->ClientName(pKiller->GetCid()));
		return;
	}

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "You are spectator until '%s' dies", Server()->ClientName(pKiller->GetCid()));
	GameServer()->SendChatTarget(pVictim->GetCid(), aBuf);

	pVictim->m_KillerId = pKiller->GetCid();
	UpdateCatchTicks(pVictim, ECatchUpdate::CAUGHT);
	m_pDeadSpecController->KillPlayer(pVictim, pKiller->GetCid());

	int Found = count(pKiller->m_vVictimIds.begin(), pKiller->m_vVictimIds.end(), pVictim->GetCid());
	if(!Found)
	{
		pKiller->m_vVictimIds.emplace_back(pVictim->GetCid());
		if(KillCounts)
			AddToKillsThatCount(pKiller, 1);
	}
}

void CGameControllerZcatch::OnCaught(class CPlayer *pVictim, class CPlayer *pKiller)
{
	if(pVictim->GetCid() == pKiller->GetCid())
		return;

	if(CatchGameState() == ECatchGameState::WAITING_FOR_PLAYERS)
	{
		if(!pKiller->m_GotRespawnInfo)
			GameServer()->SendChatTarget(pKiller->GetCid(), "Kill respawned because there are not enough players.");
		pKiller->m_GotRespawnInfo = true;
		return;
	}
	if(CatchGameState() == ECatchGameState::RELEASE_GAME)
	{
		if(!pKiller->m_GotRespawnInfo)
			GameServer()->SendChatTarget(pKiller->GetCid(), "Kill respawned because this is a release game.");
		pKiller->m_GotRespawnInfo = true;
		return;
	}

	KillPlayer(pVictim, pKiller, true);
}

int CGameControllerZcatch::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	if(pKiller && GameState() != IGS_END_ROUND && WeaponId != WEAPON_GAME)
	{
		// +1 extra score point for killing players
		// that already made kills
		// https://github.com/ddnet-insta/ddnet-insta/issues/274#issuecomment-2668575613
		if(pVictim->GetPlayer()->m_Spree && pKiller != pVictim->GetPlayer())
			pKiller->IncrementScore();
		// -2 extra score punishment in zCatch for dying in world
		// or suicide
		//
		// this is also called on disconnect but it is fine
		// because the disconnecting player will be saved first
		// and then punished
		// so the value is never persisted in the database
		if(pKiller == pVictim->GetPlayer())
			pKiller->AddScore(-2);
	}

	CGameControllerInstagib::OnCharacterDeath(pVictim, pKiller, WeaponId);
	ResetKillsThatCount(pVictim->GetPlayer());

	// TODO: revisit this edge case when zcatch is done
	//       a killer leaving while the bullet is flying
	if(!pKiller)
		return 0;

	// do not release players when the winner leaves during round end
	if(GameState() == IGS_END_ROUND)
		return 0;

	OnCaught(pVictim->GetPlayer(), pKiller);

	char aBuf[512];
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->m_KillerId == -1)
			continue;
		if(pPlayer->GetCid() == pVictim->GetPlayer()->GetCid())
			continue;
		if(pPlayer->m_KillerId != pVictim->GetPlayer()->GetCid())
			continue;

		// victim's victims
		str_format(aBuf, sizeof(aBuf), "You respawned because '%s' died", Server()->ClientName(pVictim->GetPlayer()->GetCid()));
		ReleasePlayer(pPlayer, aBuf);
	}

	pVictim->GetPlayer()->m_vVictimIds.clear();
	return 0;
}

void CGameControllerZcatch::YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_format(pMsg, MsgLen, "You will join the spectators once '%s' dies", Server()->ClientName(pPlayer->m_KillerId));
}

void CGameControllerZcatch::YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_format(pMsg, MsgLen, "You will join the game once '%s' dies", Server()->ClientName(pPlayer->m_KillerId));
}

bool CGameControllerZcatch::CanStillJoinDeadSpecGame(const CPlayer *pPlayerOrNullptr, char *pMsg, size_t MsgLen)
{
	if(!CGameControllerInstagib::CanStillJoinDeadSpecGame(pPlayerOrNullptr, pMsg, MsgLen))
		return false;

	if(!IsCatchGameRunning())
		return true;
	if(!IsGameRunning())
		return true;

	CPlayer *pBestPlayer = PlayerWithMostKillsThatCount();
	if(!pBestPlayer)
		return true;

	// this method is called to pick a team on join
	// in that case the player we pick a team for is NULL
	//
	// but it is also called when alive spectator decides to start
	// playing again
	//
	// We handle these cases differently. If someone joins the game
	// they stay spec as soon as the best player made at least 1 kill.
	// To avoid bypassing being caught by reconnecting.
	//
	// But if a player was waiting alive in spectators they can still
	// enter later in the round.
	bool WasAliveSpectator = pPlayerOrNullptr != nullptr;
	if(!WasAliveSpectator)
	{
		// WARNING: this message is never shown because we do not show CanJoinTeam
		//          errors on connect and this branch is only hit on connect
		//          but it is still nice to write a proper reason here for debugging
		if(pMsg)
		{
			str_format(
				pMsg,
				MsgLen,
				"'%s' already made %d kills, please wait.",
				Server()->ClientName(pBestPlayer->GetCid()),
				pBestPlayer->m_KillsThatCount);
		}
		return false;
	}

	// allow specs to join the game before the best player is too far
	if(pBestPlayer->m_KillsThatCount < 4)
		return true;

	if(pMsg)
	{
		str_format(
			pMsg,
			MsgLen,
			"'%s' already made %d kills, please wait.",
			Server()->ClientName(pBestPlayer->GetCid()),
			pBestPlayer->m_KillsThatCount);
	}
	return false;
}

void CGameControllerZcatch::DoTeamChange(CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	CGameControllerInstagib::DoTeamChange(pPlayer, Team, DoChatMsg);

	if(Team != pPlayer->GetTeam())
	{
		if(g_Config.m_SvDebugCatch)
		{
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "team change '%s' new=%d old=%d", Server()->ClientName(pPlayer->GetCid()), Team, pPlayer->GetTeam());
			SendChat(-1, TEAM_ALL, aBuf);
		}

		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			UpdateCatchTicks(pPlayer, ECatchUpdate::SPECTATE);
		else if(pPlayer->GetTeam() == TEAM_RED)
			UpdateCatchTicks(pPlayer, ECatchUpdate::CONNECT);
		else
		{
			// TODO: does not get triggered by "set_team 1 1" because the team is clamped based on IsTeamPlay in ClampTeam()
			dbg_assert(false, "player tried to join invalid team. zcatch only supports team red and team spectators");
		}
	}

	CheckChangeGameState();
}

bool CGameControllerZcatch::CheckChangeGameState()
{
	int ActivePlayers = NumActivePlayers();

	if(ActivePlayers >= MIN_ZCATCH_PLAYERS && CatchGameState() == ECatchGameState::WAITING_FOR_PLAYERS)
	{
		if(!g_Config.m_SvZcatchRequireMultipleIpsToStart || NumConnectedIps() >= MIN_ZCATCH_PLAYERS)
		{
			SendChatTarget(-1, "Enough players connected. Starting game!");
			SetCatchGameState(ECatchGameState::RUNNING);
		}
		return true;
	}
	else if(ActivePlayers < MIN_ZCATCH_PLAYERS && CatchGameState() == ECatchGameState::RUNNING)
	{
		CPlayer *pBestPlayer = PlayerWithMostKillsThatCount();
		// not enough players alive to win the round
		if(!pBestPlayer || pBestPlayer->m_KillsThatCount + NumNonDeadActivePlayers() < MIN_ZCATCH_KILLS)
		{
			SendChatTarget(-1, "Not enough players connected anymore. Starting release game.");
			SetCatchGameState(ECatchGameState::WAITING_FOR_PLAYERS);
			return true;
		}
	}
	return false;
}

void CGameControllerZcatch::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerInstagib::OnPlayerConnect(pPlayer);

	UpdateCatchTicks(pPlayer, ECatchUpdate::CONNECT);

	CPlayer *pBestPlayer = PlayerWithMostKillsThatCount();
	if(pBestPlayer && IsCatchGameRunning() && IsGameRunning())
	{
		KillPlayer(pPlayer, GameServer()->m_apPlayers[pBestPlayer->GetCid()], false);

		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' is now spectating you (selfkill to release them)", Server()->ClientName(pPlayer->GetCid()));
		SendChatTarget(pBestPlayer->GetCid(), aBuf);
	}
	else if(pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		// this should only be hit if sv_tournament_mode is 1 as far as i understand
		// in that case we join as alive spectators and users can switch to in game
		// if they want to
		UpdateCatchTicks(pPlayer, ECatchUpdate::SPECTATE);
	}

	pPlayer->m_SkinInfoManager.SetUseCustomColor(ESkinPrio::LOW, true);
	SetCatchColors(pPlayer);

	if(!CheckChangeGameState())
	{
		if(CatchGameState() == ECatchGameState::WAITING_FOR_PLAYERS)
			SendChatTarget(pPlayer->GetCid(), "Waiting for more players to start the round.");
		else if(CatchGameState() == ECatchGameState::RELEASE_GAME)
			SendChatTarget(pPlayer->GetCid(), "This is a release game.");
	}
}

void CGameControllerZcatch::OnPlayerDisconnect(class CPlayer *pDisconnectingPlayer, const char *pReason)
{
	UpdateCatchTicks(pDisconnectingPlayer, ECatchUpdate::DISCONNECT);

	CGameControllerInstagib::OnPlayerDisconnect(pDisconnectingPlayer, pReason);

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_vVictimIds.erase(std::remove(pPlayer->m_vVictimIds.begin(), pPlayer->m_vVictimIds.end(), pDisconnectingPlayer->GetCid()), pPlayer->m_vVictimIds.end());
	}

	CheckChangeGameState();
}

bool CGameControllerZcatch::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerInstagib::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
	return false;
}

bool CGameControllerZcatch::DoWincheckRound()
{
	// also allow winning by reaching scorelimit
	// if sv_scorelimit is set
	// https://github.com/ddnet-insta/ddnet-insta/issues/274
	if(IGameController::DoWincheckRound())
		return true;

	if(IsCatchGameRunning() && NumNonDeadActivePlayers() <= 1)
	{
		bool GotWinner = false;
		for(CPlayer *pPlayer : GameServer()->m_apPlayers)
		{
			if(!pPlayer)
				continue;

			// this player ended the round
			if(IsWinner(pPlayer, nullptr, 0))
			{
				char aBuf[512];
				int WinPoints = WinPointsForWin(pPlayer);
				// if stat track is on points are given in the base controller
				// if it is off we still want to give points
				// because you can only win if you made enough kills
				// if by then everyone left you should still be rewarded
				if(!IsStatTrack())
					pPlayer->m_Stats.m_WinPoints += WinPoints;
				str_format(aBuf, sizeof(aBuf), "'%s' won the round and gained %d points.", Server()->ClientName(pPlayer->GetCid()), WinPoints);
				SendChat(-1, TEAM_ALL, aBuf);
				GotWinner = true;
			}
		}
		if(!GotWinner)
		{
			SendChatTarget(-1, "Nobody won. Starting release game.");
			SetCatchGameState(ECatchGameState::WAITING_FOR_PLAYERS);
			ReleaseAllPlayers();
			return false;
		}

		EndRound();
		ReleaseAllPlayers();

		return true;
	}
	return false;
}

void CGameControllerZcatch::ReleaseAllPlayers()
{
	m_pDeadSpecController->RespawnAllPlayers();

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_KillerId = -1;
		pPlayer->m_vVictimIds.clear();
	}
}

void CGameControllerZcatch::AddToKillsThatCount(CPlayer *pPlayer, int Kills)
{
	pPlayer->m_KillsThatCount += Kills;
	SetCatchColors(pPlayer);
}

void CGameControllerZcatch::ResetKillsThatCount(CPlayer *pPlayer)
{
	pPlayer->m_KillsThatCount = 0;
	SetCatchColors(pPlayer);
}

CPlayer *CGameControllerZcatch::PlayerWithMostKillsThatCount()
{
	CPlayer *pBestKiller = nullptr;
	int HighestCount = 0;
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(pPlayer->m_KillsThatCount <= HighestCount)
			continue;

		pBestKiller = pPlayer;
		HighestCount = pPlayer->m_KillsThatCount;
	}
	return pBestKiller;
}

REGISTER_GAMEMODE(zcatch, CGameControllerZcatch(pGameServer));
