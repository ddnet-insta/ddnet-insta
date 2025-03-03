// ddnet-insta specific gamecontroller methods
#include <base/log.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/shared/protocolglue.h>
#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/teamscore.h>

#include <game/server/entities/character.h>
#include <game/server/entities/door.h>
#include <game/server/entities/dragger.h>
#include <game/server/entities/flag.h>
#include <game/server/entities/gun.h>
#include <game/server/entities/light.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/projectile.h>

#include <game/server/gamecontroller.h>

// ddnet-insta

void IGameController::OnDDRaceTimeLoad(class CPlayer *pPlayer, float Time)
{
	pPlayer->m_Score = Time;
}

int IGameController::SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore)
{
	if(Server()->IsSixup(SnappingClient))
	{
		// Times are in milliseconds for 0.7
		return pPlayer->m_Score.has_value() ? GameServer()->Score()->PlayerData(pPlayer->GetCid())->m_BestTime * 1000 : -1;
	}

	return DDRaceScore;
}

int IGameController::SnapRoundStartTick(int SnappingClient)
{
	if(
		!Server()->IsSixup(SnappingClient) &&
		(GameState() == IGS_START_COUNTDOWN_ROUND_START || GameState() == IGS_START_COUNTDOWN_UNPAUSE))
	{
		return m_UnpauseStartTick;
	}

	return m_RoundStartTick;
}

int IGameController::SnapTimeLimit(int SnappingClient)
{
	if(
		!Server()->IsSixup(SnappingClient) &&
		(GameState() == IGS_START_COUNTDOWN_ROUND_START || GameState() == IGS_START_COUNTDOWN_UNPAUSE))
	{
		// magic number of 20 minutes fake timelimit
		// if this is changed the place where m_UnpauseStartTick is set also has to be changed
		// the countdown configs have a maximum of 1000 seconds each
		// so 20 minutes is enough to display a countdown using the timelimit
		return 20;
	}

	return g_Config.m_SvTimelimit;
}

int IGameController::GetCarriedFlag(CPlayer *pPlayer)
{
	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return FLAG_NONE;

	for(int FlagIndex = FLAG_RED; FlagIndex < NUM_FLAGS; FlagIndex++)
		if(m_apFlags[FlagIndex] && m_apFlags[FlagIndex]->GetCarrier() == pChr)
			return FlagIndex;
	return FLAG_NONE;
}

CClientMask IGameController::FreezeDamageIndicatorMask(class CCharacter *pChr)
{
	return pChr->TeamMask() & GameServer()->ClientsMaskExcludeClientVersionAndHigher(VERSION_DDNET_NEW_HUD);
}

int IGameController::FreeInGameSlots()
{
	// TODO: add SvPlayerSlots in upstream

	int Players = m_aTeamSize[TEAM_RED] + m_aTeamSize[TEAM_BLUE];
	int Slots = Server()->MaxClients() - g_Config.m_SvSpectatorSlots;
	return maximum(0, Slots - Players);
}

int IGameController::GetPlayerTeam(CPlayer *pPlayer, bool Sixup)
{
	return pPlayer->GetTeam();
}

bool IGameController::IsPlaying(const CPlayer *pPlayer)
{
	return pPlayer->GetTeam() == TEAM_RED || pPlayer->GetTeam() == TEAM_BLUE;
}

void IGameController::ToggleGamePause()
{
	SetPlayersReadyState(false);
	if(GameServer()->m_World.m_Paused)
		SetGameState(IGS_GAME_RUNNING);
	else
		SetGameState(IGS_GAME_PAUSED, TIMER_INFINITE);
}

void IGameController::AddTeamscore(int Team, int Score)
{
	if(IsWarmup())
		return;

	m_aTeamscore[Team] += Score;
}

int IGameController::WinPointsForWin(const CPlayer *pPlayer)
{
	// in non team based modes everyone is an enemy
	// if you manage to win you get one point for
	// every in game player
	if(!IsTeamPlay())
		return m_aTeamSize[TEAM_RED] + m_aTeamSize[TEAM_BLUE];

	// if it is a team based mode you get a point for every member of
	// the enemy team
	//
	// not super sure how much sense this actually makes
	// in a 3v3 the win can be less your responsibility
	// than in a 2v2 but you would get more points.
	//
	// It can be argued that higher amounts of players make the games
	// them self more valuable. They are also harder to farm.
	// Even if the individuals participation might have less impact.
	return m_aTeamSize[pPlayer->GetTeam() == TEAM_RED ? TEAM_BLUE : TEAM_RED];
}

// balancing
void IGameController::CheckTeamBalance()
{
	if(!IsTeamPlay() || !Config()->m_SvTeambalanceTime)
	{
		m_UnbalancedTick = TBALANCE_OK;
		return;
	}

	// check if teams are unbalanced
	if(absolute(m_aTeamSize[TEAM_RED] - m_aTeamSize[TEAM_BLUE]) >= protocol7::NUM_TEAMS)
	{
		log_info("game", "Teams are NOT balanced (red=%d blue=%d)", m_aTeamSize[TEAM_RED], m_aTeamSize[TEAM_BLUE]);
		if(m_UnbalancedTick <= TBALANCE_OK)
			m_UnbalancedTick = Server()->Tick();
	}
	else
	{
		log_info("game", "Teams are balanced (red=%d blue=%d)", m_aTeamSize[TEAM_RED], m_aTeamSize[TEAM_BLUE]);
		m_UnbalancedTick = TBALANCE_OK;
	}
}

void IGameController::DoTeamBalance()
{
	if(!IsTeamPlay() || absolute(m_aTeamSize[TEAM_RED] - m_aTeamSize[TEAM_BLUE]) < protocol7::NUM_TEAMS)
		return;
	// if(m_GameFlags&GAMEFLAG_SURVIVAL)
	// 	return;

	log_info("game", "Balancing teams");

	float aTeamScore[protocol7::NUM_TEAMS] = {0};
	float aPlayerScore[MAX_CLIENTS] = {0.0f};

	// gather stats
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		{
			aPlayerScore[i] = GameServer()->m_apPlayers[i]->m_Score.value_or(0) * Server()->TickSpeed() * 60.0f /
					  (Server()->Tick() - GameServer()->m_apPlayers[i]->m_ScoreStartTick);
			aTeamScore[GameServer()->m_apPlayers[i]->GetTeam()] += aPlayerScore[i];
		}
	}

	int BiggerTeam = (m_aTeamSize[TEAM_RED] > m_aTeamSize[TEAM_BLUE]) ? TEAM_RED : TEAM_BLUE;
	int NumBalance = absolute(m_aTeamSize[TEAM_RED] - m_aTeamSize[TEAM_BLUE]) / protocol7::NUM_TEAMS;

	// balance teams
	do
	{
		CPlayer *pPlayer = nullptr;
		float ScoreDiff = aTeamScore[BiggerTeam];
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!GameServer()->m_apPlayers[i] || !CanBeMovedOnBalance(i))
				continue;

			// remember the player whom would cause lowest score-difference
			if(GameServer()->m_apPlayers[i]->GetTeam() == BiggerTeam &&
				(!pPlayer || absolute((aTeamScore[BiggerTeam ^ 1] + aPlayerScore[i]) - (aTeamScore[BiggerTeam] - aPlayerScore[i])) < ScoreDiff))
			{
				pPlayer = GameServer()->m_apPlayers[i];
				ScoreDiff = absolute((aTeamScore[BiggerTeam ^ 1] + aPlayerScore[i]) - (aTeamScore[BiggerTeam] - aPlayerScore[i]));
			}
		}

		// move the player to the other team
		if(pPlayer)
		{
			int Temp = pPlayer->m_LastActionTick;
			DoTeamChange(pPlayer, BiggerTeam ^ 1);
			pPlayer->m_LastActionTick = Temp;
			pPlayer->Respawn();
			GameServer()->SendGameMsg(protocol7::GAMEMSG_TEAM_BALANCE_VICTIM, pPlayer->GetTeam(), pPlayer->GetCid());
		}
	} while(--NumBalance);

	m_UnbalancedTick = TBALANCE_OK;
	GameServer()->SendGameMsg(protocol7::GAMEMSG_TEAM_BALANCE, -1);
}

void IGameController::AmmoRegen(CCharacter *pChr)
{
	pChr->AmmoRegen();
}

bool IGameController::IsPlayerReadyMode()
{
	return Config()->m_SvPlayerReadyMode != 0 && (m_GameStateTimer == TIMER_INFINITE && (m_GameState == IGS_WARMUP_USER || m_GameState == IGS_GAME_PAUSED));
}

void IGameController::OnPlayerReadyChange(CPlayer *pPlayer)
{
	if(pPlayer->m_LastReadyChangeTick && pPlayer->m_LastReadyChangeTick + Server()->TickSpeed() * 1 > Server()->Tick())
		return;

	pPlayer->m_LastReadyChangeTick = Server()->Tick();

	if(Config()->m_SvPlayerReadyMode && pPlayer->GetTeam() != TEAM_SPECTATORS && !pPlayer->m_DeadSpecMode)
	{
		// change players ready state
		pPlayer->m_IsReadyToPlay ^= 1;

		if(m_GameState == IGS_GAME_RUNNING && !pPlayer->m_IsReadyToPlay)
		{
			SetGameState(IGS_GAME_PAUSED, TIMER_INFINITE); // one player isn't ready -> pause the game
			GameServer()->SendGameMsg(protocol7::GAMEMSG_GAME_PAUSED, pPlayer->GetCid(), -1);
		}

		GameServer()->PlayerReadyStateBroadcast();
		CheckReadyStates();
	}
	else
		GameServer()->PlayerReadyStateBroadcast();
}

// to be called when a player changes state, spectates or disconnects
void IGameController::CheckReadyStates(int WithoutId)
{
	if(Config()->m_SvPlayerReadyMode)
	{
		switch(m_GameState)
		{
		case IGS_GAME_PAUSED:
			// all players are ready -> unpause the game
			if(GetPlayersReadyState(WithoutId))
			{
				SetGameState(IGS_GAME_PAUSED, 0);
				GameServer()->SendBroadcastSix("", false); // clear "%d players not ready" 0.6 backport
			}
			break;
		case IGS_GAME_RUNNING:
		case IGS_WARMUP_USER:
		case IGS_WARMUP_GAME:
		case IGS_START_COUNTDOWN_UNPAUSE:
		case IGS_START_COUNTDOWN_ROUND_START:
		case IGS_END_ROUND:
			// not affected
			break;
		}
	}
}

bool IGameController::GetPlayersReadyState(int WithoutId, int *pNumUnready)
{
	int Unready = 0;
	for(int i = 0; i < Server()->MaxClients(); ++i)
	{
		if(i == WithoutId)
			continue; // skip
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS && !GameServer()->m_apPlayers[i]->m_IsReadyToPlay)
		{
			if(!pNumUnready)
				return false;
			Unready++;
		}
	}
	if(pNumUnready)
		*pNumUnready = Unready;

	return true;
}

void IGameController::SetPlayersReadyState(bool ReadyState)
{
	for(auto &pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS && (ReadyState || !pPlayer->m_DeadSpecMode))
			pPlayer->m_IsReadyToPlay = ReadyState;
}

bool IGameController::DoWincheckRound()
{
	if(IsWarmup())
		return false;

	if(IsTeamPlay())
	{
		// check score win condition
		if((m_GameInfo.m_ScoreLimit > 0 && (m_aTeamscore[TEAM_RED] >= m_GameInfo.m_ScoreLimit || m_aTeamscore[TEAM_BLUE] >= m_GameInfo.m_ScoreLimit)) ||
			(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
		{
			if(m_aTeamscore[TEAM_RED] != m_aTeamscore[TEAM_BLUE] || m_GameFlags & protocol7::GAMEFLAG_SURVIVAL)
			{
				EndRound();
				return true;
			}
			else
				m_SuddenDeath = 1;
		}
	}
	else
	{
		// gather some stats
		int Topscore = 0;
		int TopscoreCount = 0;
		for(auto &pPlayer : GameServer()->m_apPlayers)
		{
			if(!pPlayer)
				continue;
			int Score = pPlayer->m_Score.value_or(0);
			if(Score > Topscore)
			{
				Topscore = Score;
				TopscoreCount = 1;
			}
			else if(Score == Topscore)
				TopscoreCount++;
		}

		// check score win condition
		if((m_GameInfo.m_ScoreLimit > 0 && Topscore >= m_GameInfo.m_ScoreLimit) ||
			(m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60))
		{
			if(TopscoreCount == 1)
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

void IGameController::CheckGameInfo()
{
	bool GameInfoChanged = (m_GameInfo.m_ScoreLimit != g_Config.m_SvScorelimit) || (m_GameInfo.m_TimeLimit != g_Config.m_SvTimelimit);
	m_GameInfo.m_MatchCurrent = 0;
	m_GameInfo.m_MatchNum = 0;
	m_GameInfo.m_ScoreLimit = g_Config.m_SvScorelimit;
	m_GameInfo.m_TimeLimit = g_Config.m_SvTimelimit;
	if(GameInfoChanged)
		SendGameInfo(-1);
}

bool IGameController::IsFriendlyFire(int ClientId1, int ClientId2)
{
	if(ClientId1 == ClientId2)
		return false;

	if(IsTeamPlay())
	{
		if(!GameServer()->m_apPlayers[ClientId1] || !GameServer()->m_apPlayers[ClientId2])
			return false;

		// if(!Config()->m_SvTeamdamage && GameServer()->m_apPlayers[ClientId1]->GetTeam() == GameServer()->m_apPlayers[ClientId2]->GetTeam())
		if(true && GameServer()->m_apPlayers[ClientId1]->GetTeam() == GameServer()->m_apPlayers[ClientId2]->GetTeam())
			return true;
	}

	return false;
}

void IGameController::SendGameInfo(int ClientId)
{
	// ddnet-insta
	for(int i = 0; i < Server()->MaxClients(); i++)
	{
		if(ClientId != -1)
			if(ClientId != i)
				continue;

		if(Server()->IsSixup(i))
		{
			protocol7::CNetMsg_Sv_GameInfo Msg;
			Msg.m_GameFlags = m_GameFlags;
			Msg.m_MatchCurrent = 1;
			Msg.m_MatchNum = 0;
			Msg.m_ScoreLimit = Config()->m_SvScorelimit;
			Msg.m_TimeLimit = Config()->m_SvTimelimit;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, i);
		}
	}
}

void IGameController::SetGameState(EGameState GameState, int Timer)
{
	// change game state
	switch(GameState)
	{
	case IGS_WARMUP_GAME:
		// game based warmup is only possible when game or any warmup is running
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER)
		{
			if(Timer == TIMER_INFINITE)
			{
				// run warmup till there're enough players
				m_GameState = GameState;
				m_GameStateTimer = TIMER_INFINITE;

				// enable respawning in survival when activating warmup
				// if(m_GameFlags&GAMEFLAG_SURVIVAL)
				// {
				// 	for(int i = 0; i < Server()->MaxClients(); ++i)
				// 		if(GameServer()->m_apPlayers[i])
				// 			GameServer()->m_apPlayers[i]->m_RespawnDisabled = false;
				// }
			}
			else if(Timer == 0)
			{
				// start new match
				StartRound();
			}
			m_GamePauseStartTime = -1;
		}
		break;
	case IGS_WARMUP_USER:
		// user based warmup is only possible when the game or any warmup is running
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED || m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER)
		{
			if(Timer != 0)
			{
				// start warmup
				if(Timer < 0)
				{
					m_Warmup = 0;
					m_GameState = GameState;
					m_GameStateTimer = TIMER_INFINITE;
					if(Config()->m_SvPlayerReadyMode)
					{
						// run warmup till all players are ready
						SetPlayersReadyState(false);
					}
				}
				else if(Timer > 0)
				{
					// run warmup for a specific time intervall
					m_GameState = GameState;
					m_GameStateTimer = Timer * Server()->TickSpeed(); // TODO: this is vanilla timer
					m_Warmup = Timer * Server()->TickSpeed(); // TODO: this is ddnet timer
				}

				// enable respawning in survival when activating warmup
				// if(m_GameFlags&GAMEFLAG_SURVIVAL)
				// {
				// 	for(int i = 0; i < Server()->MaxClients(); ++i)
				// 		if(GameServer()->m_apPlayers[i])
				// 			GameServer()->m_apPlayers[i]->m_RespawnDisabled = false;
				// }
				GameServer()->m_World.m_Paused = false;
			}
			else
			{
				// start new match
				StartRound();
			}
			m_GamePauseStartTime = -1;
		}
		break;
	case IGS_START_COUNTDOWN_ROUND_START:
	case IGS_START_COUNTDOWN_UNPAUSE:
		// only possible when game, pause or start countdown is running
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED || m_GameState == IGS_START_COUNTDOWN_ROUND_START || m_GameState == IGS_START_COUNTDOWN_UNPAUSE)
		{
			int CountDownSeconds = 0;
			if(m_GameState == IGS_GAME_PAUSED)
				CountDownSeconds = Config()->m_SvCountdownUnpause;
			else
				CountDownSeconds = Config()->m_SvCountdownRoundStart;

			// there will be a fake 20 minute time limit for 0.6 clients
			// so we also send a fake start tick that is CountDownSeconds before
			// the 20 minute timelimit would expire

			int FakeTimeLimitEndTick = Server()->Tick() - Server()->TickSpeed() * 60 * 20;
			m_UnpauseStartTick = FakeTimeLimitEndTick + (CountDownSeconds * Server()->TickSpeed());

			// m_UnpauseStartTick = Server()->Tick() - Server()->TickSpeed() * 30;

			if(CountDownSeconds == 0 && m_GameFlags & protocol7::GAMEFLAG_SURVIVAL)
			{
				m_GameState = GameState;
				m_GameStateTimer = 3 * Server()->TickSpeed();
				GameServer()->m_World.m_Paused = true;
			}
			else if(CountDownSeconds > 0)
			{
				m_GameState = GameState;
				m_GameStateTimer = CountDownSeconds * Server()->TickSpeed();
				GameServer()->m_World.m_Paused = true;
			}
			else
			{
				// no countdown, start new match right away
				SetGameState(IGS_GAME_RUNNING);
			}
			// m_GamePauseStartTime = -1; // countdown while paused still counts as paused
		}
		break;
	case IGS_GAME_RUNNING:
		// always possible
		{
			// ddnet-insta specific
			// vanilla does not do this
			// but ddnet sends m_RoundStartTick in snap
			// so we have to also update that to show current game timer
			if(m_GameState == IGS_START_COUNTDOWN_ROUND_START || m_GameState == IGS_GAME_RUNNING)
			{
				m_RoundStartTick = Server()->Tick();
			}
			// this is also ddnet-insta specific
			// no idea how vanilla does it
			// but this solves countdown delaying timelimit end
			// meaning that if countdown and timelimit is set the
			// timerstops at 00:00 and waits the additional countdown time
			// before actually ending the game
			// https://github.com/ddnet-insta/ddnet-insta/issues/41
			if(m_GameState == IGS_START_COUNTDOWN_ROUND_START)
			{
				m_GameStartTick = Server()->Tick();
				dbg_msg("ddnet-insta", "reset m_GameStartTick");
			}
			m_Warmup = 0;
			m_GameState = GameState;
			m_GameStateTimer = TIMER_INFINITE;
			SetPlayersReadyState(true);
			GameServer()->m_World.m_Paused = false;
			m_GamePauseStartTime = -1;
		}
		break;
	case IGS_GAME_PAUSED:
		// only possible when game is running or paused, or when game based warmup is running
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_GAME_PAUSED || m_GameState == IGS_WARMUP_GAME)
		{
			// 0.6 clients get the scoreboard forced on them when the game is paused
			// so we track them on pause to use chat instead of broadcast to target them
			for(CPlayer *pPlayer : GameServer()->m_apPlayers)
			{
				if(!pPlayer)
					continue;

				pPlayer->m_HasGhostCharInGame = pPlayer->GetCharacter() != 0;
			}

			if(Timer != 0)
			{
				// start pause
				if(Timer < 0)
				{
					// pauses infinitely till all players are ready or disabled via rcon command
					m_GameStateTimer = TIMER_INFINITE;
					SetPlayersReadyState(false);
				}
				else
				{
					// pauses for a specific time interval
					m_GameStateTimer = Timer * Server()->TickSpeed();
				}

				m_GameState = GameState;
				GameServer()->m_World.m_Paused = true;
			}
			else
			{
				// start a countdown to end pause
				SetGameState(IGS_START_COUNTDOWN_UNPAUSE);
			}
			m_GamePauseStartTime = time_get();
		}
		break;
	case IGS_END_ROUND:
		if(m_Warmup) // game can't end when we are running warmup
			break;
		m_GamePauseStartTime = -1;
		m_GameOverTick = Server()->Tick();
		if(m_GameState == IGS_END_ROUND)
			break;
		// only possible when game is running or over
		if(m_GameState == IGS_GAME_RUNNING || m_GameState == IGS_END_ROUND || m_GameState == IGS_GAME_PAUSED)
		{
			m_GameState = GameState;
			m_GameStateTimer = Timer * Server()->TickSpeed();
			// m_GameOverTick = Timer * Server()->Tick();
			m_SuddenDeath = 0;
			GameServer()->m_World.m_Paused = true;

			OnRoundEnd(); // ddnet-insta specific
		}
	}
}

void IGameController::OnFlagReturn(CFlag *pFlag)
{
}

void IGameController::OnFlagGrab(CFlag *pFlag)
{
}

void IGameController::OnFlagCapture(CFlag *pFlag, float Time, int TimeTicks)
{
}

int IGameController::GetCidByName(const char *pName)
{
	int ClientId = -1;
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(!str_comp(pName, Server()->ClientName(pPlayer->GetCid())))
		{
			ClientId = pPlayer->GetCid();
			break;
		}
	}
	return ClientId;
}

void IGameController::SetArmorProgressFull(CCharacter *pCharacer)
{
	pCharacer->SetArmor(10);
}

void IGameController::SetArmorProgressEmpty(CCharacter *pCharacer)
{
	pCharacer->SetArmor(0);
}

bool IGameController::HasWinningScore(const CPlayer *pPlayer) const
{
	if(IsTeamPlay())
	{
		if(pPlayer->GetTeam() < TEAM_RED || pPlayer->GetTeam() > TEAM_BLUE)
			return false;
		return m_aTeamscore[pPlayer->GetTeam()] > m_aTeamscore[!pPlayer->GetTeam()];
	}
	else
	{
		int OwnScore = pPlayer->m_Score.value_or(0);
		if(!OwnScore)
			return false;

		int Topscore = 0;
		for(auto &pOtherPlayer : GameServer()->m_apPlayers)
		{
			if(!pOtherPlayer)
				continue;
			int Score = pOtherPlayer->m_Score.value_or(0);
			if(Score > Topscore)
				Topscore = Score;
		}
		return OwnScore >= Topscore;
	}

	return false;
}
