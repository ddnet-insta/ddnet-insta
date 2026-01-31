#ifndef INSTA_SERVER_ROUND_STATS_PLAYER_H
#define INSTA_SERVER_ROUND_STATS_PLAYER_H

// See also `CSqlStatsPlayer` for stats that get stored in the database
//
// The round stats get reset on round start and are tracked at all times.
// They have overlapping values with the sql stats. But sql stats are
// tracked conditionally to avoid farming. Round stats are always tracked.
class CRoundStatsPlayer
{
public:
	int m_Kills;
	int m_Deaths;

	// total hooks launched
	int m_Hooks;

	// this is a subset of m_Hooks
	// counting all hooks that did not grab
	// a tile or a player
	int m_HooksMissed;

	// this is a subset of m_Hooks
	// counting all hooks that attached to another player
	int m_HooksHitPlayer;

	// how many own dropped flags *this* player touched
	// and returned them to the own base
	int m_FlagReturns;

	// Called on round end/start and player join
	void Reset()
	{
		m_Kills = 0;
		m_Deaths = 0;
		m_Hooks = 0;
		m_HooksMissed = 0;
		m_HooksHitPlayer = 0;
		m_FlagReturns = 0;
	}

	CRoundStatsPlayer()
	{
		Reset();
	}
};

#endif
