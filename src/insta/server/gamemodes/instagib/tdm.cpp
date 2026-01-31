#include "tdm.h"

#include <game/server/entities/character.h>
#include <game/server/player.h>

CGameControllerInstaTDM::CGameControllerInstaTDM(class CGameContext *pGameServer) :
	CGameControllerInstaBaseDM(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS;
}

CGameControllerInstaTDM::~CGameControllerInstaTDM() = default;

void CGameControllerInstaTDM::Tick()
{
	CGameControllerInstaBaseDM::Tick();
}

int CGameControllerInstaTDM::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	if(pKiller && WeaponId != WEAPON_GAME)
	{
		// do team scoring
		if(pKiller == pVictim->GetPlayer() || pKiller->GetTeam() == pVictim->GetPlayer()->GetTeam())
			AddTeamscore(pKiller->GetTeam() & 1, -1);
		else
			AddTeamscore(pKiller->GetTeam() & 1, 1);
	}

	return CGameControllerBasePvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
}
