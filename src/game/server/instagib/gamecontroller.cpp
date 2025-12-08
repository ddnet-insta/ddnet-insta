// ddnet-insta specific gamecontroller methods
#include <base/log.h>
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/protocolglue.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/door.h>
#include <game/server/entities/dragger.h>
#include <game/server/entities/gun.h>
#include <game/server/entities/light.h>
#include <game/server/entities/pickup.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/entities/flag.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/teamscore.h>

void IGameController::OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg)
{
	if(Killer != WEAPON_GAME && pVictim->m_SetSavePos[RESCUEMODE_AUTO])
		pVictim->GetPlayer()->m_LastDeath = pVictim->m_RescueTee[RESCUEMODE_AUTO];
	pVictim->StopRecording();
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(pVictim, (Killer < 0) ? nullptr : GameServer()->m_apPlayers[Killer], Weapon);

	LogKillMessage(pVictim, Killer, Weapon, ModeSpecial);

	if(SendKillMsg)
	{
		SendDeathInfoMessage(pVictim, Killer, Weapon, ModeSpecial);
	}

	// a nice sound
	// and exploding tee death effect
	SendDeathEvent(pVictim, Killer, Weapon);

	// this is to rate limit respawning to 3 secs
	pVictim->m_pPlayer->m_PreviousDieTick = pVictim->m_pPlayer->m_DieTick;
	pVictim->m_pPlayer->m_DieTick = Server()->Tick();

	pVictim->m_Alive = false;
	pVictim->SetSolo(false);

	GameServer()->m_World.RemoveEntity(pVictim);
	GameServer()->m_World.m_Core.m_apCharacters[pVictim->m_pPlayer->GetCid()] = nullptr;
	pVictim->Teams()->OnCharacterDeath(pVictim->GetPlayer()->GetCid(), Weapon);
	pVictim->CancelSwapRequests();
}

void IGameController::SendDeathInfoMessage(CCharacter *pVictim, int Killer, int Weapon, int ModeSpecial)
{
	pVictim->SendDeathMessageIfNotInLockedTeam(Killer, Weapon, ModeSpecial);
}

void IGameController::SendDeathEvent(CCharacter *pVictim, int Killer, int Weapon)
{
	GameServer()->CreateSound(pVictim->m_Pos, SOUND_PLAYER_DIE, pVictim->TeamMask());
	GameServer()->CreateDeath(pVictim->m_Pos, pVictim->m_pPlayer->GetCid(), pVictim->TeamMask());
}

void IGameController::LogKillMessage(CCharacter *pVictim, int Killer, int Weapon, int ModeSpecial)
{
	// ddnet-insta added this branch
	// inspired by upstream https://github.com/teeworlds/teeworlds/blob/5d682733e482950f686663c129adc4b751c8d790/src/game/server/entities/character.cpp#L665
	// to fix a crash bug
	if(Killer < 0 || !GameServer()->m_apPlayers[Killer])
	{
		log_info("game", "kill killer='%d:%d:' victim='%d:%s' weapon=%d special=%d",
			Killer, -1 - Killer,
			pVictim->GetPlayer()->GetCid(), Server()->ClientName(pVictim->GetPlayer()->GetCid()), Weapon, ModeSpecial);
	}
	else
	{
		log_info("game", "kill killer='%d:%s' victim='%d:%s' weapon=%d special=%d",
			Killer, Server()->ClientName(Killer),
			pVictim->GetPlayer()->GetCid(), Server()->ClientName(pVictim->GetPlayer()->GetCid()), Weapon, ModeSpecial);
	}
}

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
	if(Team != TEAM_RED && Team != TEAM_BLUE)
		return;

	m_aTeamscore[Team] += Score;
}

int IGameController::WinPointsForWin(const CPlayer *pPlayer)
{
	// The amount of win points used to depend on the amount of
	// enemies. But that does make little sense for most
	// modes. Team based or not. The amount of enemies
	// does not convey the value of the win.
	// If it is a dm mode with scorelimit.
	// The best player will usually win.
	// And reaching the scorelimit should require the same
	// time and difficulty with one enemy or with 100 enemies.
	// The chance of winning is stastically lower with
	// more competitors but I don't think thats relevant here (the more balanced the skill is the more relevant it is).
	// In team modes getting carried in a 8v8 and barely contributing
	// to the game should not reward you more for the 8 enemies
	// than doing 50% of the work in 2v2.
	//
	// In zCatch that is different. Here every enemy has to be killed.
	// Winning does get harder with every player that joins.
	// So the zCatch mode overwrites this method.
	//
	// I decided the players score on round end is the best metric
	// and it works surprisingly well for all current gamemodes team based
	// or not.
	// The players score (can vary from the scorelimit!) best reflects
	// how much was contributed to the win in a team mode. More or less at least, team play can be complex ofc.
	// And also how much time and effort had to be put in in a solo mode.
	//
	// But thinking about it again the amount of enemies is actually relevant.
	// Because of the before mentioned chance that it includes players that are stronger (at least in non team modes).
	// But also because of anti farming reasons. Farming wins at night with a dummy and
	// friends in an almost empty server should be rewarded less
	// than winning on a full server.
	int Points = pPlayer->m_Score.value_or(0);

	// this enemie amount is rigged on public servers
	// more correct would be the average player count during the entire game
	// players come and go
	//
	// TODO:
	// Also it should exclude players in ddrace teams see
	// https://github.com/ddnet-insta/ddnet-insta/issues/350
	int Enemies = m_aTeamSize[TEAM_RED] + m_aTeamSize[TEAM_BLUE];
	if(IsTeamPlay())
		Enemies = m_aTeamSize[pPlayer->GetTeam() == TEAM_RED ? TEAM_BLUE : TEAM_RED];

	// Not sure if enemies should be counted if team play is on
	// assuming the teams are balanced there are only two possible winners
	// So red or blue win chance is kind of 50% in a 1vs1 and in a 8vs8.
	//
	// Rewarding ctf1 players less for a win than ctf3 players seems a bit odd.
	// So the main motivation here is to make win points harder to farm.
	// Because bigger games are usually harder to farm or fill with friends.
	//
	// The amount of enemies is not weight strongly anyways.
	// Usually the amount of points is higher than the amount of players.
	return Enemies + Points;
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
	// ready change can only be used to pause and unpause the game
	// during warmup and countdown and round end it should never have any effect
	// otherwise we can get into a bad state where only half the players are ready
	// but the game is actually running
	// in ddnet-insta the game should be fully paused as soon as one player is not ready
	// ready changes are not used to skip the warmup phase!
	//
	// https://github.com/ddnet-insta/ddnet-insta/issues/331
	if(m_GameState != IGS_GAME_RUNNING && m_GameState != IGS_GAME_PAUSED)
		return;
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

bool IGameController::IsFriendlyFire(int ClientId1, int ClientId2) const
{
	if(ClientId1 == ClientId2)
		return false;

	if(IsTeamPlay())
	{
		if(!GameServer()->m_apPlayers[ClientId1] || !GameServer()->m_apPlayers[ClientId2])
			return false;

		if(!g_Config.m_SvTeamdamage && GameServer()->m_apPlayers[ClientId1]->GetTeam() == GameServer()->m_apPlayers[ClientId2]->GetTeam())
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
					// run warmup for a specific time interval
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

CPlayer *IGameController::GetPlayerOrNullptr(int ClientId) const
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return nullptr;

	return GameServer()->m_apPlayers[ClientId];
}

void IGameController::SetArmorProgressFull(CCharacter *pCharacter)
{
	pCharacter->SetArmor(10);
}

void IGameController::SetArmorProgressEmpty(CCharacter *pCharacter)
{
	pCharacter->SetArmor(0);
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

void IGameController::OnPauseChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	if(IsDDRaceGameType())
		CGameContext::ConTogglePause(pResult, pUserData);
	else
		CGameContext::ConReadyChange(pResult, pUserData);
}

void IGameController::OnSpecChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	if(IsDDRaceGameType())
	{
		CGameContext::ConToggleSpec(pResult, pUserData);
	}
	else
	{
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", "Command not available in this gametype.");
	}
}

void IGameController::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	CGameContext::ConCredits(pResult, pUserData);
}
