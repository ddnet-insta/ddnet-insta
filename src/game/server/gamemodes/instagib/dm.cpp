#include "dm.h"

#include <game/server/entities/character.h>

CGameControllerInstaBaseDM::CGameControllerInstaBaseDM(class CGameContext *pGameServer) :
	CGameControllerInstagib(pGameServer)
{
	m_GameFlags = 0;
}

CGameControllerInstaBaseDM::~CGameControllerInstaBaseDM() = default;

void CGameControllerInstaBaseDM::Tick()
{
	CGameControllerBasePvp::Tick();
}

void CGameControllerInstaBaseDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBasePvp::OnCharacterSpawn(pChr);
}

int CGameControllerInstaBaseDM::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	return CGameControllerBasePvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
}
