#include "team_fng.h"

#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

CGameControllerTeamFng::CGameControllerTeamFng(class CGameContext *pGameServer) :
	CGameControllerBaseFng(pGameServer)
{
	// this is a base gamemode
	// full implementations are "fng" and "boomfng"
	m_GameFlags = GAMEFLAG_TEAMS;
}

CGameControllerTeamFng::~CGameControllerTeamFng() = default;

void CGameControllerTeamFng::Tick()
{
	CGameControllerBaseFng::Tick();
}

void CGameControllerTeamFng::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBaseFng::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);
}

int CGameControllerTeamFng::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId)
{
	CGameControllerBaseFng::OnCharacterDeath(pVictim, pKiller, WeaponId);
	return 0;
}

bool CGameControllerTeamFng::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	CGameControllerBaseFng::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
	return false;
}
