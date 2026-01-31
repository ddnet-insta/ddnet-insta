#ifndef INSTA_SERVER_ENUMS_H
#define INSTA_SERVER_ENUMS_H

#include <generated/protocol.h>

enum
{
	FLAG_NONE = -1,
	FLAG_RED = 0,
	FLAG_BLUE = 1,
	NUM_FLAGS = 2
};

enum
{
	// ddnet-insta special weapon
	// not supported by the protocol or client
	WEAPON_HOOK = WEAPON_GAME - 1,
};

// ddnet-insta can show text in the world
// this is used for displaying scores in the fng
// gametypes (see the config `sv_text_points`)
//
// this enum determines the object that will be used
// to draw the text.
enum class ETextType
{
	// meta value to represent the text being off
	NONE,

	// This uses the round laser/rifle texture
	// that is placed at the end of a laser or at the position
	// where it bends on a wallshot.
	// So on a default setup teeworlds or ddnet client
	// this will be a blue text.
	LASER,

	// This technically uses the `WEAPON_HAMMER` projectile texture.
	// Which is a hack because the hammer actually does not shoot projectiles.
	// But the network protocol allows it as a valid weapon.
	// In game this will look like a soft white circle.
	// It is basically the texture that makes up the gun and shotgun trail.
	PROJECTILE,
};

enum class EAllowed
{
	YES,
	NO,
	LATER,
};

enum class EDisplayScore
{
	ROUND_POINTS,
	// all time points from database
	POINTS,
	// all time best spree
	SPREE,
	CURRENT_SPREE,
	WIN_POINTS,
	WINS,
	KILLS,
	ROUND_KILLS,

	NUM_SCORES,
};

// The in game scoreboard and the master server player list
// can have a points or time score.
enum class EScoreKind
{
	TIME,
	POINTS,
};

#define DISPLAY_SCORE_VALUES "points, round_points, spree, current_spree, win_points, wins, kills, round_kills"

// writes based on the input pInputText the output pDisplayScore
// returns true on match
// returns false on no match
bool str_to_display_score(const char *pInputText, EDisplayScore *pDisplayScore);

const char *display_score_to_str(EDisplayScore Score);

#endif
