#ifndef GAME_SERVER_INSTAGIB_GAMECONTROLLER_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_IGAMECONTROLLER

#include <base/vmath.h>
#include <engine/map.h>
#include <engine/shared/protocol.h>
#include <game/server/teams.h>

#include <engine/shared/http.h> // ddnet-insta
#include <game/generated/protocol.h>
#include <game/generated/protocol7.h>

#include <game/server/gamecontext.h>
#include <game/server/instagib/enums.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/instagib/sql_stats_player.h>

struct CScoreLoadBestTimeResult;

class IGameController
{
#endif // IN_CLASS_IGAMECONTROLLER

public:
	//      _     _            _        _           _
	//   __| | __| |_ __   ___| |_     (_)_ __  ___| |_ __ _
	//  / _` |/ _` | '_ \ / _ \ __|____| | '_ \/ __| __/ _` |
	// | (_| | (_| | | | |  __/ ||_____| | | | \__ \ || (_| |
	//  \__,_|\__,_|_| |_|\___|\__|    |_|_| |_|___/\__\__,_|
	//
	// all code below should be ddnet-insta
	//

	// virtual bool OnLaserHitCharacter(vec2 From, vec2 To, class CLaser &Laser) {};
	/*
		Function: OnCharacterTakeDamage
			this function was added in ddnet-insta and is a non standard controller method.
			neither ddnet nor teeworlds have this

		Returns:
			return true to skip ddnet CCharacter::TakeDamage() behavior
			which is applying the force and moving the damaged tee
			it also sets the happy eyes if the Dmg is not zero
	*/
	virtual bool OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character) { return false; };

	/*
		Function: OnInit
			Will be called at the end of CGameContext::OnInit
			and can be used in addition to the controllers constructor

			Its main use case is running code in a base controller after its child constructor
	*/
	virtual void OnInit(){};

	/*
		Function: ForceNetworkClipping
			Will be called on snap. Can be used to force remove entities from the snapshot.
			But can not be used to force add entities to the snapshot.

		Arguments:
			pEntity - entity that will or will not be included in the snapshot
			SnappingClient - ClientId of the player receiving the snapshot
			CheckPos - position of the entity

		Returns:
			true - to not include this entity in the snapshot for SnappingClient
			false - to let the ddnet code decide if clipping happens or not
	*/
	virtual bool ForceNetworkClipping(const CEntity *pEntity, int SnappingClient, vec2 CheckPos) { return false; }

	/*
		Function: ForceNetworkClippingLine
			Will be called on snap. Can be used to force remove entities from the snapshot.
			But can not be used to force add entities to the snapshot.

		Arguments:
			pEntity - entity that will or will not be included in the snapshot
			SnappingClient - ClientId of the player receiving the snapshot
			StartPos - start position of the line the entity is located at
			EndPos - end position of the line the enitity is located at

		Returns:
			true - to not include this entity in the snapshot for SnappingClient
			false - to let the ddnet code decide if clipping happens or not
	*/
	virtual bool ForceNetworkClippingLine(const CEntity *pEntity, int SnappingClient, vec2 StartPos, vec2 EndPos) { return false; }

	/*
		Function: OnClientDataPersist
			Will be called before map changes.
			Store your data here that you want to keep across map changes.
			To extended the data struct have a look at the file

			src/game/server/instagib/persistent_client_data.h

			And to load the data again you have to also implement
			OnClientDataRestore()

		Arguments:
			pPlayer - the player that is about to be destroyed (read from here)
			pData - the struct that can store the values across map changes (write to this)
	*/
	virtual void OnClientDataPersist(CPlayer *pPlayer, CGameContext::CPersistentClientData *pData){};

	/*
		Function: OnClientDataRestore
			Will be called on map load. But only for players
			that have persisted data from the last map change.

			Will be called before map changes.
			Store your data here that you want to keep across map changes.
			To extended the data struct have a look at the file

			src/game/server/instagib/persistent_client_data.h

			And to load the data again you have to also implement
			OnClientDataRestore()

		Arguments:
			pPlayer - the player that is about to be destroyed (write to this)
			pData - the struct that can store the values across map changes (read from here)
	*/
	virtual void OnClientDataRestore(CPlayer *pPlayer, const CGameContext::CPersistentClientData *pData){};

	/*
		Function: OnRoundStart
			Will be called after OnInit when the server first launches
			Will also be called on the beginning of every round

			Beginning of a round is defined as after the warmup but before the countdown.

			For example if "sv_countdown_round_start" is set to "10"
			and someone runs the bang command "!restart 5" in chat
			this will happen:
				- 5 seconds warmup everyone can move around and warmup
				- OnRoundStart() is called
				- 10 seconds world is paused and there is a final countdown
				- all tees will be respawned and the game starts
	*/
	virtual void OnRoundStart(){};

	/*
		Function: OnRoundEnd
			Will be called at the beginning of the end of every round.
			If you need to run code after the waiting time in the death screen
			consider using `OnRoundStart()`
	*/
	virtual void OnRoundEnd(){};

	/*
		Function: OnLaserHit
			Will be called before Character::TakeDamage() and CGameController::OnCharacterTakeDamage()

			this function was added in ddnet-insta and is a non standard controller method.
			neither ddnet nor teeworlds have this

		Arguments:
			Bounces - 1 and more is a wallshot
			From - client id of the player who shot the laser
			Weapon - probably either WEAPON_LASER or WEAPON_SHOTGUN
			pVictim - character that was hit

		Returns:
			true - to call TakeDamage
			false - to skip TakeDamage
	*/
	virtual bool OnLaserHit(int Bounces, int From, int Weapon, CCharacter *pVictim) { return true; };

	/*
		Function: OnFireWeapon
			this function was added in ddnet-insta and is a non standard controller method.
			neither ddnet nor teeworlds have this

		Returns:
			return true to skip ddnet CCharacter::FireWeapon() behavior
			which is doing standard ddnet fire weapon things
	*/
	virtual bool OnFireWeapon(CCharacter &Character, int &Weapon, vec2 &Direction, vec2 &MouseTarget, vec2 &ProjStartPos) { return false; };

	/*
		Function: OnChatMessage
			hooks into CGameContext::OnSayNetMessage()
			after unicode check and teehistorian already happend

		Returns:
			return true to not run the rest of CGameContext::OnSayNetMessage()
			which would print it to the chat or run it as a ddrace chat command
	*/
	virtual bool OnChatMessage(const CNetMsg_Cl_Say *pMsg, int Length, int &Team, CPlayer *pPlayer) { return false; };

	/*
		Function: OnChangeInfoNetMessage
			hooks into CGameContext::OnChangeInfoNetMessage()
			after spam protection check

		Returns:
			return true to not run the rest of CGameContext::OnChangeInfoNetMessage()
	*/
	virtual bool OnChangeInfoNetMessage(const CNetMsg_Cl_ChangeInfo *pMsg, int ClientId) { return false; }

	/*
		Function: OnSkinChange7
			gets run if a 0.7 client requested a skin change
			after spam protection check

		Returns:
			return true to skip the default behavior and consume the event
	*/
	virtual bool OnSkinChange7(protocol7::CNetMsg_Cl_SkinChange *pMsg, int ClientId) { return false; }

	/*
		Function: OnSetTeamNetMessage
			hooks into CGameContext::OnSetTeamNetMessage()
			before any spam protection check

			See also CanJoinTeam() which is called after the validation

		Returns:
			return true to not run the rest of CGameContext::OnSetTeamNetMessage()
	*/
	virtual bool OnSetTeamNetMessage(const CNetMsg_Cl_SetTeam *pMsg, int ClientId) { return false; };

	/*
		Function: OnCallVoteNetMessage
			hooks into CGameContext::OnCallVoteNetMessage()
			before any spam protection check

			This is being called when a player creates a new vote

			See also `OnVoteNetMessage()`

		Returns:
			return true to not run the rest of CGameContext::OnCallVoteNetMessage()
	*/
	virtual bool OnCallVoteNetMessage(const CNetMsg_Cl_CallVote *pMsg, int ClientId)
	{
		return false;
	}

	/*
		Function: OnVoteNetMessage
			hooks into CGameContext::OnVoteNetMessage()
			before any spam protection check

			This is being called when a player votes yes or no.

			See also `OnCallVoteNetMessage()`

		Returns:
			return true to not run the rest of CGameContext::OnVoteNetMessage()
	*/
	virtual bool OnVoteNetMessage(const CNetMsg_Cl_Vote *pMsg, int ClientId) { return false; }

	/*
		Function: GetPlayerTeam
			wraps CPlayer::GetTeam()
			to spoof fake teams for different versions
			this can be used to place players into spec for 0.6 and dead spec for 0.7

		Arguments:
			Sixup - will be true if that team value is sent to a 0.7 connection and false otherwise

		Returns:
			as integer TEAM_RED, TEAM_BLUE or TEAM_SPECTATORS
	*/
	virtual int GetPlayerTeam(class CPlayer *pPlayer, bool Sixup);

	/*
		Function: GetDefaultWeapon
			Returns the weapon the tee should spawn with.
			Is not a complete list of all weapons the tee gets on spawn.

			The complete list of weapons depends on the active controller and what it sets in
			its OnCharacterSpawn() if it is a gamemode without fixed weapons
			it depends on sv_spawn_weapons then it will call IGameController:SetSpawnWeapons()
	*/
	virtual int GetDefaultWeapon(class CPlayer *pPlayer) { return WEAPON_GUN; }

	/*
		Function: SetSpawnWeapons
			Is empty by default because ddnet and many ddnet-insta modes cover that in
			IGameController::OnCharacterSpawn()
			This method was added to set spawn weapons independently of the gamecontroller
			so we can use the same zCatch controller for laser and grenade zCatch

			It is also different from GetDefaultWeapon() because it could set more then one weapon.

			All gamemodes that allow different type of spawn weapons should call SetSpawnWeapons()
			in their OnCharacterSpawn() hook and also set the default weapon to GetDefaultWeaponBasedOnSpawnWeapons()
	*/
	virtual void SetSpawnWeapons(class CCharacter *pChr){};

	/*
		Function: UpdateSpawnWeapons
			called when the config sv_spawn_weapons is updated
			to update the internal enum

		Arguments:
			Silent - if false it might print warnings to the admin console
			Apply - if false it has no effect. Used to make sure spawn weapons only get changed on reload.
	*/
	virtual void UpdateSpawnWeapons(bool Silent = false, bool Apply = false){};

	/*
		Function: IsWinner
			called on disconnect and round end
			used to track stats

		Arguments:
			pPlayer - the player to check
			pMessage - should be sent to pPlayer in chat contains messages such as "you gained one win", "this win did not count because xyz"
			SizeOfMessage - size of the message buffer
	*/
	virtual bool IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage) { return false; }

	/*
		Function: IsLoser
			called on disconnect and round end
			used to track stats

		Arguments:
			pPlayer - the player to check
	*/
	virtual bool IsLoser(const CPlayer *pPlayer) { return false; }

	/*
		Function: WinPointsForWin
			Computes the amount of win points for winning a round.
			"win points" are points you can only get by winning.
			By default the reward will be higher if you had more enemies.
			These are WinPoints are not to be confused with regular round Points.

		Arguments:
			pPlayer - the player that won
	*/
	virtual int WinPointsForWin(const CPlayer *pPlayer);

	/*
		Function: IsPlaying
			Should return true if the player is playing. But the player does not have to
			be alive. And might be currently in team spectators (for example LMS/zCatch).

			Should return false if the player is intentionally spectating
			and not participating in the game at all.

			This replaces the pPlayer->m_Team == TEAM_SPECTATORS check because it supports
			also dead players and any other situtations where players that are technically not
			just watching the game end up in the spectator team for a short period of time.

		Arguments:
			pPlayer - the player to check
	*/
	virtual bool IsPlaying(const CPlayer *pPlayer);

	/*
		Function: OnShowStatsAll
			called from the main thread when a SQL worker finished querying stats from the database

		Arguments:
			pStats - stats struct to display
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName){};

	/*
		Function: OnShowRoundStats
			called when /stats command is executed
			print your gamemode specific round stats here

		Arguments:
			pStats - stats struct to display
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName){};

	/*
		Function: OnShowMultis
			called from the main thread when a SQL worker finished querying stats from the database
			called when someone uses the /multis chat command

		Arguments:
			pStats - stats struct to display
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowMultis(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName){};

	/*
		Function: OnLoadedNameStats
			Called when the stats request finished that fetches the
			stats for players that just connected or changed their name

			This can be used for save servers to display the players
			all time stats in the scoreboard

		Arguments:
			pStats - stats struct that was loaded
			pPlayer - player the stats are from
	*/
	virtual void OnLoadedNameStats(const CSqlStatsPlayer *pStats, class CPlayer *pPlayer){};

	/*
		Function: OnShowRank
			called from the main thread when a SQL worker finished querying a rank from the database

		Arguments:
			Rank - is the rank the player got with its score compared to all other players (lower is better)
			RankedScore - is the score that was used to obtain the rank if its ranking kills this will be the amount of kills
			pRankType - is the displayable string that shows the type of ranks (for example "Kills")
			pRequestingPlayer - player who initiated the stats request (might differ from the requested player)
			pRequestedName - player name the stats belong to
	*/
	virtual void OnShowRank(
		int Rank,
		int RankedScore,
		const char *pRankType,
		class CPlayer *pRequestingPlayer,
		const char *pRequestedName){};

	/*
		Function: IsStatTrack
			Called before stats changed.
			If this returns false the stats will not be updated.
			This is used to protect against farming. Define for example a minium amount of in game players
			required to count the stats.

		Arguments:
			pReason - reason buffer for stat track being off
			SizeOfReason - reason buffer size

		Returns:
			true - count stats
			false - do not count stats
	*/
	virtual bool IsStatTrack(char *pReason = nullptr, int SizeOfReason = 0)
	{
		if(pReason)
			pReason[0] = '\0';
		return true;
	}

	/*
		Function: SaveStatsOnRoundEnd
			Called for every player on round end once
			the base_pvp controller implements stats saving
			you probably do not need to extend this.
			If a player leaves before round end the method
			SaveStatsOnDisconnect() will be called.

		Arguments:
			pPlayer - player to save stats for
	*/
	virtual void SaveStatsOnRoundEnd(CPlayer *pPlayer){};

	/*
		Function: SaveStatsOnDisconnect
			Called for every player that leaves the game
			unless the game state is in round end
			then SaveStatsOnRoundEnd() was already called

		Arguments:
			pPlayer - player to save stats for
	*/
	virtual void SaveStatsOnDisconnect(CPlayer *pPlayer){};
	/*
		Function: LoadNewPlayerNameData
			Similar to ddnets LoadPlayerData()
			Called on player connect and name change
			used to load stats for that name

		Arguments:
			ClientId - id of the player to load the stats for

		Returns:
			return true to not run any ddrace time loading code
	*/
	virtual bool LoadNewPlayerNameData(int ClientId) { return false; };
	virtual void OnPlayerReadyChange(class CPlayer *pPlayer); // 0.7 ready change
	virtual int SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags) { return DDRaceFlags; };
	virtual int SnapGameInfoExFlags2(int SnappingClient, int DDRaceFlags) { return DDRaceFlags; };

	/*
		Function: SnapPlayerFlags7
			Set custom player flags for 0.7 connections.

		Arguments:
			SnappingClient - Client Id of the player that will receive the snapshot
			pPlayer - CPlayer that is being snapped
			PlayerFlags7 - the flags that were already set for that player by ddnet

		Returns:
			return the new flags value that should be snapped to the SnappingClient
	*/
	virtual int SnapPlayerFlags7(int SnappingClient, CPlayer *pPlayer, int PlayerFlags7) { return PlayerFlags7; };

	/*
		Function: SnapPlayer6
			Alter snap values for 0.6 snapshots.
			For 0.7 use `SnapPlayerFlags7()` and `SnapPlayerScore()`

			Be careful with setting `pPlayerInfo->m_Score` to not overwrite
			what `SnapPlayerScore()` tries to set.

		Arguments:
			SnappingClient - Client Id of the player that will receive the snapshot
			pPlayer - CPlayer that is being snapped
			pClientInfo - (in and output) info that is being snappend which is already pre filled by ddnet and can be altered.
			pPlayerInfo - (in and output) info that is being snappend which is already pre filled by ddnet and can be altered.
	*/
	virtual void SnapPlayer6(int SnappingClient, CPlayer *pPlayer, CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo){};

	/*
		Function: SnapPlayerScore
			Warning its value could be overwritten by `SnapPlayer6()`

		Arguments:
			SnappingClient - Client Id of the player that will receive the snapshot
			pPlayer - CPlayer that is being snapped
			DDRaceScore - Current value of the score set by the ddnet code

		Returns:
			return the new score value that will be included in the snapshot
	*/
	virtual int SnapPlayerScore(int SnappingClient, CPlayer *pPlayer, int DDRaceScore);
	virtual void SnapDDNetCharacter(int SnappingClient, CCharacter *pChr, CNetObj_DDNetCharacter *pDDNetCharacter){};
	virtual void SnapDDNetPlayer(int SnappingClient, CPlayer *pPlayer, CNetObj_DDNetPlayer *pDDNetPlayer){};
	virtual int SnapRoundStartTick(int SnappingClient);
	virtual int SnapTimeLimit(int SnappingClient);

	/*
		Function: GetCarriedFlag
			Returns the type of flag the given player is currently carrying.
			Flag refers here to a CTF gametype flag which is either red, blue or none.

		Arguments:
			pPlayer - player to check

		Returns:
			FLAG_NONE -1
			FLAG_RED  0
			FLAG_BLUE 2
	*/
	virtual int GetCarriedFlag(class CPlayer *pPlayer);

	/*
		Function: InitPlayer
			Called once for every new CPlayer object that is being constructed
			is only called when a new player connects
			not on round end.
			See also `RoundInitPlayer()`

		Arguments:
			pPlayer - newly joined player
	*/
	virtual void InitPlayer(class CPlayer *pPlayer){};

	/*
		Function: RoundInitPlayer
			Called for all players when a new round starts
			And also for all players that join
			See also `InitPlayer()`

		Arguments:
			pPlayer - player that was connected on round start
	*/
	virtual void RoundInitPlayer(class CPlayer *pPlayer){};

	/*
		Function: FreeInGameSlots
			The amount of free in game slots.
			Used to block players from joining the game if
			for example a 1vs1 one is running and already
			2 players are playing.

			In ddnet-insta this value is more complex than
			looking at read red + team blue and the SvPlayerSlots / SvSpectatorSlots config
			because we also have game modes such as zCatch
			where players can be spectators during the time they are dead
			but they are still considered active players
			while there are also permanent spectators that do not
			occupy any slots.

			Call this method if you want to know how many players can still join the game.
			And overwrite it if you have a more custom demand to count these than
			looking at players that are not spectators.

			If this method returns 0 players will see this error in the broadcast
			"Only %d active players are allowed"
	*/
	virtual int FreeInGameSlots();
	virtual CClientMask FreezeDamageIndicatorMask(CCharacter *pChr);
	virtual void OnDDRaceTimeLoad(class CPlayer *pPlayer, float Time);

	// See also ddnet's SetArmorProgress() and ddnet-insta's SetArmorProgressEmpty()
	// used to keep armor progress bar in ddnet gametype
	// but remove it in favor of correct armor in vanilla based gametypes
	virtual void SetArmorProgressFull(CCharacter *pCharacer);

	// See also ddnet's SetArmorProgress() and ddnet-insta's SetArmorProgressFull()
	// used to keep armor progress bar in ddnet gametype
	// but remove it in favor of correct armor in vanilla based gametypes
	virtual void SetArmorProgressEmpty(CCharacter *pCharacer);

	// ddnet has grenade
	// but the actual implementation is in CGameControllerPvp::IsGrenadeGameType()
	virtual bool IsGrenadeGameType() const { return true; }
	virtual bool IsFngGameType() const { return false; }
	virtual bool IsZcatchGameType() const { return false; }
	bool IsVanillaGameType() const { return m_IsVanillaGameType; }
	virtual bool IsDDRaceGameType() const { return true; }
	bool m_IsVanillaGameType = false;
	int m_DefaultWeapon = WEAPON_GUN;
	void CheckReadyStates(int WithoutId = -1);
	bool GetPlayersReadyState(int WithoutId = -1, int *pNumUnready = nullptr);
	void SetPlayersReadyState(bool ReadyState);
	bool IsPlayerReadyMode();
	void ToggleGamePause();
	void AbortWarmup()
	{
		if((m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER) && m_GameStateTimer != TIMER_INFINITE)
		{
			SetGameState(IGS_GAME_RUNNING);
		}
	}
	void SwapTeamscore()
	{
		if(!IsTeamPlay())
			return;

		int Score = m_aTeamscore[TEAM_RED];
		m_aTeamscore[TEAM_RED] = m_aTeamscore[TEAM_BLUE];
		m_aTeamscore[TEAM_BLUE] = Score;
	};

	void AddTeamscore(int Team, int Score);

	int m_aTeamSize[protocol7::NUM_TEAMS];

	// game
	enum EGameState
	{
		// internal game states
		IGS_WARMUP_GAME, // warmup started by game because there're not enough players (infinite)
		IGS_WARMUP_USER, // warmup started by user action via rcon or new match (infinite or timer)

		IGS_START_COUNTDOWN_ROUND_START, // start countown to start match/round (tick timer)
		IGS_START_COUNTDOWN_UNPAUSE, // start countown to unpause the game

		IGS_GAME_PAUSED, // game paused (infinite or tick timer)
		IGS_GAME_RUNNING, // game running (infinite)

		IGS_END_ROUND, // round is over (tick timer)
	};
	EGameState m_GameState;
	EGameState GameState() const { return m_GameState; }
	bool IsWarmup() const { return m_GameState == IGS_WARMUP_GAME || m_GameState == IGS_WARMUP_USER; }
	bool IsInfiniteWarmup() const { return IsWarmup() && m_GameStateTimer == TIMER_INFINITE; }
	int IsGameRunning() const { return m_GameState == IGS_GAME_RUNNING; }
	int IsGameCountdown() const { return m_GameState == IGS_START_COUNTDOWN_ROUND_START || m_GameState == IGS_START_COUNTDOWN_UNPAUSE; }
	int m_GameStateTimer;

	enum EWinType
	{
		// First player or team to reach sv_scorelimit wins.
		WIN_BY_SCORE,

		// Last player or team to stay alive wins.
		WIN_BY_SURVIVAL,
	};

	EWinType m_WinType = WIN_BY_SCORE;

	// What is the determining factor to win the game.
	// Most game modes require reaching the sv_scorelimit.
	// But some also just look at who stays alive until the end.
	EWinType WinType() const { return m_WinType; }

	// custom ddnet-insta timers
	int m_UnpauseStartTick = 0;

	const char *GameStateToStr(EGameState GameState)
	{
		switch(GameState)
		{
		case IGS_WARMUP_GAME:
			return "IGS_WARMUP_GAME";
		case IGS_WARMUP_USER:
			return "IGS_WARMUP_USER";
		case IGS_START_COUNTDOWN_ROUND_START:
			return "IGS_START_COUNTDOWN_ROUND_START";
		case IGS_START_COUNTDOWN_UNPAUSE:
			return "IGS_START_COUNTDOWN_UNPAUSE";
		case IGS_GAME_PAUSED:
			return "IGS_GAME_PAUSED";
		case IGS_GAME_RUNNING:
			return "IGS_GAME_RUNNING";
		case IGS_END_ROUND:
			return "IGS_END_ROUND";
		}
		return "UNKNOWN";
	}

	bool HasEnoughPlayers() const { return (IsTeamPlay() && m_aTeamSize[TEAM_RED] > 0 && m_aTeamSize[TEAM_BLUE] > 0) || (!IsTeamPlay() && m_aTeamSize[TEAM_RED] > 1); }
	void SetGameState(EGameState GameState, int Timer = 0);

	bool m_AllowSkinColorChange = true;

	// protected:
public:
	struct CGameInfo
	{
		int m_MatchCurrent;
		int m_MatchNum;
		int m_ScoreLimit;
		int m_TimeLimit;
	} m_GameInfo;
	void SendGameInfo(int ClientId);
	/*
		Variable: m_GameStartTick
			Sent in snap to 0.7 clients for timer
	*/
	int m_GameStartTick;
	int m_aTeamscore[protocol7::NUM_TEAMS];

	void EndRound() { SetGameState(IGS_END_ROUND, TIMER_END); }

	float CalcKillDeathRatio(int Kills, int Deaths) const;

	void GetRoundEndStatsStrCsv(char *pBuf, size_t Size);
	void GetRoundEndStatsStrCsvTeamPlay(char *pBuf, size_t Size);
	void GetRoundEndStatsStrCsvNoTeamPlay(char *pBuf, size_t Size);
	void PsvRowPlayer(const CPlayer *pPlayer, char *pBuf, size_t Size);
	void GetRoundEndStatsStrJson(char *pBuf, size_t Size);
	void GetRoundEndStatsStrPsv(char *pBuf, size_t Size);
	void GetRoundEndStatsStrAsciiTable(char *pBuf, size_t SizeOfBuf);
	void GetRoundEndStatsStrHttp(char *pBuf, size_t Size);
	void GetRoundEndStatsStrDiscord(char *pBuf, size_t Size);
	void GetRoundEndStatsStrFile(char *pBuf, size_t Size);
	void PublishRoundEndStatsStrFile(const char *pStr);
	void PublishRoundEndStatsStrDiscord(const char *pStr);
	void PublishRoundEndStatsStrHttp(const char *pStr);
	void PublishRoundEndStats();

public:
	enum
	{
		TIMER_INFINITE = -1,
		TIMER_END = 10,
	};

	virtual bool DoWincheckRound(); // returns true when the match is over
	virtual void OnFlagReturn(class CFlag *pFlag); // ddnet-insta
	virtual void OnFlagGrab(class CFlag *pFlag); // ddnet-insta
	virtual void OnFlagCapture(class CFlag *pFlag, float Time, int TimeTicks); // ddnet-insta
	// return true to consume the event
	// and supress default ddnet selfkill behavior
	virtual bool OnSelfkill(int ClientId) { return false; };
	virtual void OnUpdateZcatchColorConfig(){};
	virtual void OnUpdateSpectatorVotesConfig(){};
	virtual bool DropFlag(class CCharacter *pChr) { return false; };
	virtual bool HasWinningScore(const CPlayer *pPlayer) const;

	/*
		Variable: m_GamePauseStartTime

		gets set to time_get() when a player pauses the game
		using the ready change if sv_player_ready_mode is active

		it can then be used to track how long a game has been paused already

		it is set to -1 if the game is currently not paused
	*/
	int64_t m_GamePauseStartTime;

	// if it is greater than 0 it means in that many
	// ticks the server will be shutdown
	//
	// depends on the base pvp controller to tick
	int m_TicksUntilShutdown = 0;

	bool IsSkinColorChangeAllowed() const { return m_AllowSkinColorChange; }
	int GameFlags() const { return m_GameFlags; }
	void CheckGameInfo();
	bool IsFriendlyFire(int ClientId1, int ClientId2);

	// get client id by in game name
	int GetCidByName(const char *pName);

	// only used in ctf gametypes
	class CFlag *m_apFlags[NUM_FLAGS];

	CSqlStats *m_pSqlStats = nullptr;
	const char *m_pStatsTable = "";
	const char *StatsTable() const { return m_pStatsTable; }

private:
#ifndef IN_CLASS_IGAMECONTROLLER
};
#endif
