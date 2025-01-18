#include <base/system.h>

#include "enums.h"

bool str_to_display_score(const char *pInputText, EDisplayScore *pDisplayScore)
{
	if(!pInputText && pInputText[0] == '\0')
		return false;

	if(!str_comp_nocase(pInputText, "points"))
		*pDisplayScore = EDisplayScore::POINTS;
	else if(!str_comp_nocase(pInputText, "round_points"))
		*pDisplayScore = EDisplayScore::ROUND_POINTS;
	else
		return false;
	return true;
}

const char *display_score_to_str(EDisplayScore Score)
{
	return "";
}
