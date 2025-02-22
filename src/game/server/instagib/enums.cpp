#include <base/system.h>

#include "enums.h"

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
	case EDisplayScore::NUM_SCORES:
		return "(invalid)";
	}

	return "(invalid)";
}
