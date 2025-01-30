#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "dm.h"

CGameControllerInstaBaseDM::CGameControllerInstaBaseDM(class CGameContext *pGameServer) :
	CGameControllerInstagib(pGameServer)
{
	m_GameFlags = 0;
}

CGameControllerInstaBaseDM::~CGameControllerInstaBaseDM() = default;

void CGameControllerInstaBaseDM::Tick()
{
	CGameControllerPvp::Tick();
}

void CGameControllerInstaBaseDM::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerPvp::OnCharacterSpawn(pChr);
}

int CGameControllerInstaBaseDM::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	return CGameControllerPvp::OnCharacterDeath(pVictim, pKiller, WeaponId);
}
