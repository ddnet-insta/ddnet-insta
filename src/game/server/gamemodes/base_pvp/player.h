#ifndef GAME_SERVER_GAMEMODES_BASE_PVP_PLAYER_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_PLAYER

#include <base/vmath.h>
#include <game/server/instagib/enums.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/instagib/sql_stats_player.h>
#include <optional>
#include <vector>

// player object
class CPlayer
{
	std::optional<int> m_Score; // hack for IDEs
	int m_Team;
#endif // IN_CLASS_PLAYER

public:
	void InstagibTick();

	void ProcessStatsResult(CInstaSqlResult &Result);

	int m_SentWarmupAlerts = 0;
	void WarmupAlert();
	const char *GetTeamStr() const;

	// The type of score that will be snapped to this player
	// and displayed in the scoreboard
	// it is determined by the sv_display_score config and /score chat command
	// and used by the GetDisplayScore gamecontroller method
	EDisplayScore m_DisplayScore = EDisplayScore::POINTS;

	/*******************************************************************
	 * zCatch                                                          *
	 *******************************************************************/

	// Will be -1 when the player is alive
	int m_KillerId = -1;
	void SetTeamSpoofed(int Team, bool DoChatMsg = false);
	void SetTeamNoKill(int Team, bool DoChatMsg = false);
	void SetTeamRaw(int Team) { m_Team = Team; }
	// dead players can not respawn
	// will be used like m_RespawnDisabled in 0.7
	bool m_IsDead;
	bool m_GotRespawnInfo = false;
	bool m_WantsToJoinSpectators = false;
	std::vector<int> m_vVictimIds;

	// kills made in zCatch that give reward points
	// if the player wins
	//
	// it includes players that left the game so it might be more than m_vVictimIds.size()
	// and it does not include players that were released so it is more than m_Spree
	int m_KillsThatCount = 0;

	/*******************************************************************
	 * gCTF                                                            *
	 *******************************************************************/
	bool m_GameStateBroadcast;
	int m_RespawnTick;
	bool m_IsReadyToEnter; // 0.7 ready change
	bool m_IsReadyToPlay; // 0.7 ready change
	bool m_DeadSpecMode; // 0.7 dead players

	/*******************************************************************
	 * fng                                                             *
	 *******************************************************************/
	// see also m_LastToucherId
	int m_OriginalFreezerId = -1;
	// amount of seconds to freeze on next spawn
	int m_FreezeOnSpawn = 0;

	int m_Multi = 1;

	int64_t m_LastKillTime = 0;
	int64_t HandleMulti();

	/*******************************************************************
	 * shared                                                          *
	 *******************************************************************/
	//Anticamper
	bool m_SentCampMsg;
	int m_CampTick;
	vec2 m_CampPos;

	// fng and block
	int m_LastToucherId = -1;
	void UpdateLastToucher(int ClientId);

	// ticks since m_LastToucherId was set
	// only used in block mode for now
	int m_TicksSinceLastTouch = 0;

	// Will also be set if spree chat messages are turned off
	// this is the current spree
	// not to be confused with m_Stats.m_BestSpree which is the highscore
	// it is only incremented if stat track is on (enough players connected)
	int m_Spree = 0;

	// it will only be incremented if stat track is off (not enough players connected)
	int m_UntrackedSpree = 0;

	// all metrics in m_Stats are protected by anti farm
	// and might not be incremented if not enough players are connected
	// see stats directly in CPlayer such as CPlayer::m_Kills for stats
	// that are always counted
	CSqlStatsPlayer m_Stats;

	// these are the all time stats of that player
	// loaded from the database
	// they should not be written to
	// new stats should be written to m_Stats
	// the m_SavedStats are used to display all time stats in the scoreboard
	//
	// they should never be used as source of truth for all time stats
	// because they are not synced live
	// neither for the currently connected player
	// and especially not for players connected on other servers with the same name
	// if you need the correct up to date stats of a players name you have to do a new db request
	CSqlStatsPlayer m_SavedStats;

	// currently active unterminated killing spree
	int Spree() const { return m_Spree; }

	// kills made in current round
	int Kills() const { return m_Stats.m_Kills; }

	// deaths from the current round
	int Deaths() const { return m_Stats.m_Deaths; }

	void AddKill() { AddKills(1); }
	void AddDeath() { AddDeaths(1); }
	void AddKills(int Amount);
	void AddDeaths(int Amount);

	// tracked per round no matter what
	int m_Kills = 0;
	int m_Deaths = 0;

	// resets round stats and sql stats
	void ResetStats();

	std::shared_ptr<CInstaSqlResult> m_StatsQueryResult;
	std::shared_ptr<CInstaSqlResult> m_FastcapQueryResult;

	/*
		m_HasGhostCharInGame

		when the game starts for the first time
		and then a countdown starts there are no characters in game yet
		because they had no time to spawn

		but when the game reloads and tees were in game.
		those clients do not receive a new snap and still can see their tee.
		so their scoreboard is not forced on them.

		This variable marks those in game tees
	*/
	bool m_HasGhostCharInGame;

	/*
		m_IsFakeDeadSpec

		Marks players who are sent to the client as dead specs
		but in reality are real spectators.

		This is used to enable sv_spectator_votes for 0.7
		So the 0.7 clients think they are in game but in reality they are not.
	*/
	bool m_IsFakeDeadSpec = false;
	int64_t m_LastReadyChangeTick;
	void IncrementScore() { AddScore(1); }
	void DecrementScore() { AddScore(-1); }
	void AddScore(int Score);

	// how many ticks this player sent the
	// PLAYERFLAG_CHATTING that displays the chat bubble to others
	int m_TicksSpentChatting = 0;

	// tracks if this player already got pinged in chat
	// at least once by others
	// this is used to unlock the chat for these players
	// if there is a initial chat delay
	// because automated spam bots are usually not greeted
	// and greeted players want to respond instantly
	bool m_GotPingedInChat = false;

	// if there is a anti chat spam filter active such as
	// sv_require_chat_flag_to_chat
	// then this boolean tracks players that got verified
	// to be able to use the chat
	//
	// players can also chat if the `m_GotPingedInChat` is set
	// or `m_TicksSpentChatting` is high enough
	//
	// and this boolean is for all remaining edge cases where players
	// got whitelisted because of some action they did
	// for now this is used to make sure
	// players who joined after the server was empty
	// or players who were there before a map reload
	// get whitelisted
	bool m_VerifiedForChat = false;

#ifndef IN_CLASS_PLAYER
};
#endif
