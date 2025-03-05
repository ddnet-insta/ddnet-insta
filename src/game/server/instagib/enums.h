#ifndef GAME_SERVER_INSTAGIB_ENUMS_H
#define GAME_SERVER_INSTAGIB_ENUMS_H

enum
{
	FLAG_NONE = -1,
	FLAG_RED = 0,
	FLAG_BLUE = 1,
	NUM_FLAGS = 2
};

// what does the weapon bullet cause
// on the target once it his
enum class EWeaponHitEffect {
	NONE,
	UNFREEZE,
	FREEZE,
	VANILLA_DAMAGE,
	INSTANT_KILL
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

#define DISPLAY_SCORE_VALUES "points, round_points, spree, current_spree, win_points, wins, kills, round_kills"

// writes based on the input pInputText the output pDisplayScore
// returns true on match
// returns false on no match
bool str_to_display_score(const char *pInputText, EDisplayScore *pDisplayScore);

const char *display_score_to_str(EDisplayScore Score);

#endif
