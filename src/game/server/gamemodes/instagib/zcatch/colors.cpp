#include "zcatch.h"

#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/server/entities/character.h>
#include <game/server/instagib/skin_info_manager.h>
#include <game/server/player.h>
#include <game/server/teeinfo.h>

int CGameControllerZcatch::GetBodyColorTeetime(int Kills)
{
	return (maximum(0, 160 - (Kills * 10)) * 0x010000) + 0xff00;
}

int CGameControllerZcatch::GetBodyColorSavander(int Kills)
{
	if(Kills == 0)
		return 0xFFBB00;
	if(Kills == 1)
		return 0x00FF00;
	if(Kills == 2)
		return 0x11FF00;
	if(Kills == 3)
		return 0x22FF00;
	if(Kills == 4)
		return 0x33FF00;
	if(Kills == 5)
		return 0x44FF00;
	if(Kills == 6)
		return 0x55FF00;
	if(Kills == 7)
		return 0x66FF00;
	if(Kills == 8)
		return 0x77FF00;
	if(Kills == 9)
		return 0x88FF00;
	if(Kills == 10)
		return 0x99FF00;
	if(Kills == 11)
		return 0xAAFF00;
	if(Kills == 12)
		return 0xBBFF00;
	if(Kills == 13)
		return 0xCCFF00;
	if(Kills == 14)
		return 0xDDFF00;
	if(Kills == 15)
		return 0xEEFF00;
	return 0xFFBB00;
}

int CGameControllerZcatch::GetBodyColor(int Kills)
{
	switch(m_CatchColors)
	{
	case ECatchColors::TEETIME:
		return GetBodyColorTeetime(Kills);
	case ECatchColors::SAVANDER:
		return GetBodyColorSavander(Kills);
	default:
		dbg_assert(true, "invalid color");
	}
	return 0;
}

void CGameControllerZcatch::OnUpdateZcatchColorConfig()
{
	const char *pColor = Config()->m_SvZcatchColors;

	if(!str_comp_nocase(pColor, "teetime"))
		m_CatchColors = ECatchColors::TEETIME;
	else if(!str_comp_nocase(pColor, "savander"))
		m_CatchColors = ECatchColors::SAVANDER;
	else
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "Error: invalid zcatch color scheme '%s' defaulting to 'teetime'", pColor);
		Console()->Print(
			IConsole::OUTPUT_LEVEL_STANDARD,
			"ddnet-insta",
			aBuf);
		str_copy(Config()->m_SvZcatchColors, "teetime");
	}

	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
		if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS)
			SetCatchColors(pPlayer);
}

void CGameControllerZcatch::SetCatchColors(CPlayer *pPlayer)
{
	int Color = GetBodyColor(pPlayer->m_KillsThatCount);

	// it would be cleaner if this only applied to the winner
	// we could make sure m_KillsThatCount is not reset until the next round starts
	// but for now it should work because players that connect during round end
	// will reset colors
	if(GameState() == IGS_END_ROUND)
		return;

	pPlayer->m_SkinInfoManager.SetColorBody(ESkinPrio::LOW, Color);
}
