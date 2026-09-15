#include "enums.h"

#include <base/str.h>

bool str_to_display_score(const char *pInputText, EDisplayScore *pDisplayScore)
{
	if(!pInputText || pInputText[0] == '\0')
		return false;

	if(!str_comp_nocase(pInputText, "points"))
		*pDisplayScore = EDisplayScore::POINTS;
	else if(!str_comp_nocase(pInputText, "round_points"))
		*pDisplayScore = EDisplayScore::ROUND_POINTS;
	else if(!str_comp_nocase(pInputText, "spree"))
		*pDisplayScore = EDisplayScore::SPREE;
	else if(!str_comp_nocase(pInputText, "current_spree"))
		*pDisplayScore = EDisplayScore::CURRENT_SPREE;
	else if(!str_comp_nocase(pInputText, "win_points"))
		*pDisplayScore = EDisplayScore::WIN_POINTS;
	else if(!str_comp_nocase(pInputText, "wins"))
		*pDisplayScore = EDisplayScore::WINS;
	else if(!str_comp_nocase(pInputText, "kills"))
		*pDisplayScore = EDisplayScore::KILLS;
	else if(!str_comp_nocase(pInputText, "round_kills"))
		*pDisplayScore = EDisplayScore::ROUND_KILLS;
	else if(!str_comp_nocase(pInputText, "session_points"))
		*pDisplayScore = EDisplayScore::SESSION_POINTS;
	else if(!str_comp_nocase(pInputText, "session_wins"))
		*pDisplayScore = EDisplayScore::SESSION_WINS;
	else
		return false;
	return true;
}

// TODO: use this in /score without args
//       also create a config chain and print to the admin which score type is now set
const char *display_score_to_str(EDisplayScore Score)
{
	switch(Score)
	{
	case EDisplayScore::POINTS:
		return "points";
	case EDisplayScore::ROUND_POINTS:
		return "round_points";
	case EDisplayScore::SPREE:
		return "spree";
	case EDisplayScore::CURRENT_SPREE:
		return "current_spree";
	case EDisplayScore::WIN_POINTS:
		return "win_points";
	case EDisplayScore::WINS:
		return "wins";
	case EDisplayScore::KILLS:
		return "kills";
	case EDisplayScore::ROUND_KILLS:
		return "round_kills";
	case EDisplayScore::SESSION_POINTS:
		return "session_points";
	case EDisplayScore::SESSION_WINS:
		return "session_wins";
	case EDisplayScore::NUM_SCORES:
		return "(invalid)";
	}

	return "(invalid)";
}

bool str_to_weapon(const char *pInput, int *pWeapon)
{
	if(!pInput || pInput[0] == '\0')
		return false;

	// Also support weapon ids
	int Weapon = 0;
	if(str_toint(pInput, &Weapon))
	{
		switch(Weapon)
		{
		case WEAPON_HAMMER: return Weapon;
		case WEAPON_GUN: return Weapon;
		case WEAPON_SHOTGUN: return Weapon;
		case WEAPON_GRENADE: return Weapon;
		case WEAPON_LASER: return Weapon;
		case WEAPON_NINJA: return Weapon;
		}
	}

	if(!str_comp_nocase(pInput, "hammer"))
		*pWeapon = WEAPON_HAMMER;
	else if(!str_comp_nocase(pInput, "gun"))
		*pWeapon = WEAPON_GUN;
	else if(!str_comp_nocase(pInput, "shotgun"))
		*pWeapon = WEAPON_SHOTGUN;
	else if(!str_comp_nocase(pInput, "grenade"))
		*pWeapon = WEAPON_GRENADE;
	else if(!str_comp_nocase(pInput, "laser") || !str_comp_nocase(pInput, "rifle"))
		*pWeapon = WEAPON_LASER;
	else if(!str_comp_nocase(pInput, "ninja"))
		*pWeapon = WEAPON_NINJA;
	else
		return false;

	// intentionally not supporting these
	// WEAPON_GAME = -3, // team switching etc
	// WEAPON_SELF = -2, // console kill command
	// WEAPON_WORLD = -1, // death tiles etc

	return true;
}

#define LINK_CONFIG(ConfigName, ConfigScriptName, EnumName) \
	bool str_to_##EnumName(const char *pInput, EnumName *pValue) \
	{ \
		*pValue = EnumName::
}
#include <insta/server/config_enums.h>
#undef LINK_CONFIG
