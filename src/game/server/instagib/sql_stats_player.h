#ifndef GAME_SERVER_INSTAGIB_SQL_STATS_PLAYER_H
#define GAME_SERVER_INSTAGIB_SQL_STATS_PLAYER_H

#include <base/system.h>

#include <game/server/instagib/extra_columns.h>

enum
{
	MAX_MULTIS = 11,
};

class CSqlStatsPlayer
{
public:
	// can be used as score in save servers
	// mostly matches the in game score in the scoreboard
	// also used by zCatch to give different amounts of points
	// for wins based on the amount of connected players
	int m_Points;
	// kills, deaths and flag grabs/caps are tracked per round
	int m_Kills;
	int m_Deaths;
	// in zCatch wins and losses are only counted if enough players are connected
	int m_WinPoints;
	int m_Wins;
	int m_Losses;
	// used to track accuracy in any gametype
	// in grenade, ninja and shotgun based gametypes the
	// accuracy can go over 100%
	// because one shot can have multiple hits
	//
	// hammer is excluded by default from ShotsFired and ShotsHit
	// if you need to track hammer as well you have to do so in your gamemode
	int m_ShotsFired;
	int m_ShotsHit;

	// Will also be set if spree chat messages are turned off
	// this is the spree highscore
	// the players current spree is in CPlayer::m_Spree
	int m_BestSpree;

	/*************************************
	 * GCTF                              *
	 *************************************/

	int m_FlagCaptures;
	int m_FlagGrabs;
	int m_FlaggerKills;

	/*************************************
	 * ICTF                              *
	 *************************************/

	// TODO: these are not actually incremented yet
	int m_Wallshots;

	/*************************************
	 * zCatch                            *
	 *************************************/

	int m_TicksCaught; // TODO: this is not tracked yet
	int m_TicksInGame; // TODO: this is not tracked yet

	/*************************************
	 * foot                              *
	 *************************************/

	int m_Goals;
	int m_OwnGoals;

	/*************************************
	 * bomb                              *
	 *************************************/

	int m_HammerKills;
	int m_CollateralKills;

	/*************************************
	 * solofng and fng                   *
	 *************************************/

	// the current multi is in player.h
	int m_BestMulti;

	int m_aMultis[MAX_MULTIS];

	int m_GotFrozen;
	int m_GoldSpikes;
	int m_GreenSpikes;
	int m_PurpleSpikes;

	/*************************************
	 * fng                               *
	 *************************************/

	// hammers that led to an unfreeze
	int m_Unfreezes;
	int m_WrongSpikes;
	int m_StealsByOthers;
	int m_StealsFromOthers;

	// can be over 100%
	// because of self/team boost + kill
	// or multiple kills with one shoot
	float HitAccuracy() const
	{
		if(!m_ShotsFired)
			return 0.0f;
		return ((float)m_ShotsHit / (float)m_ShotsFired) * 100;
	}

	void Reset()
	{
		// base for all gametypes
		m_Points = 0;
		m_Kills = 0;
		m_Deaths = 0;
		m_BestSpree = 0;
		m_Wins = 0;
		m_Losses = 0;
		m_WinPoints = 0;
		m_ShotsFired = 0;
		m_ShotsHit = 0;

		// gametype specific
		m_FlagCaptures = 0;
		m_FlagGrabs = 0;
		m_FlaggerKills = 0;
		m_Wallshots = 0;
		m_TicksCaught = 0;
		m_TicksInGame = 0;
		m_BestMulti = 0;
		m_GotFrozen = 0;
		m_GoldSpikes = 0;
		m_GreenSpikes = 0;
		m_PurpleSpikes = 0;
		m_HammerKills = 0;
		m_CollateralKills = 0;

		for(auto &Multi : m_aMultis)
			Multi = 0;

		m_Unfreezes = 0;
		m_WrongSpikes = 0;
		m_StealsByOthers = 0;
		m_StealsFromOthers = 0;
	}

	void Merge(const CSqlStatsPlayer *pOther)
	{
		// base for all gametypes
		m_Points += pOther->m_Points;
		m_Kills += pOther->m_Kills;
		m_Deaths += pOther->m_Deaths;
		m_BestSpree = std::max(m_BestSpree, pOther->m_BestSpree);
		m_WinPoints += pOther->m_WinPoints;
		m_Wins += pOther->m_Wins;
		m_Losses += pOther->m_Losses;
		m_ShotsFired += pOther->m_ShotsFired;
		m_ShotsHit += pOther->m_ShotsHit;

		// gametype specific is implemented in the gametypes callback
	}

	void Dump(CExtraColumns *pExtraColumns, const char *pSystem = "stats") const
	{
		dbg_msg(pSystem, "  points: %d", m_Points);
		dbg_msg(pSystem, "  kills: %d", m_Kills);
		dbg_msg(pSystem, "  deaths: %d", m_Deaths);
		dbg_msg(pSystem, "  spree: %d", m_BestSpree);
		dbg_msg(pSystem, "  win_points: %d", m_WinPoints);
		dbg_msg(pSystem, "  wins: %d", m_Wins);
		dbg_msg(pSystem, "  losses: %d", m_Losses);
		dbg_msg(pSystem, "  shots_fired: %d", m_ShotsFired);
		dbg_msg(pSystem, "  shots_hit: %d", m_ShotsHit);

		if(pExtraColumns)
			pExtraColumns->Dump(this);
	}

	bool HasValues() const
	{
		// TODO: add a HasValues callback in the gametype instead of listing all here
		return m_Points ||
		       m_Kills ||
		       m_Deaths ||
		       m_BestSpree ||
		       m_WinPoints ||
		       m_Wins ||
		       m_Losses ||
		       m_ShotsFired ||
		       m_ShotsHit ||
		       m_FlagCaptures ||
		       m_FlagGrabs ||
		       m_FlaggerKills ||
		       m_Wallshots ||
		       m_TicksCaught ||
		       m_TicksInGame ||
		       m_BestMulti ||
		       m_GotFrozen ||
		       m_GoldSpikes ||
		       m_GreenSpikes ||
		       m_PurpleSpikes ||
		       m_Unfreezes ||
		       m_WrongSpikes ||
		       m_StealsFromOthers ||
		       m_StealsByOthers;
	}

	CSqlStatsPlayer()
	{
		Reset();
	}
};

#endif
