#include "base_instagib.h"

#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

CGameControllerBaseInstagib::CGameControllerBaseInstagib(class CGameContext *pGameServer) :
	CGameControllerBasePvp(pGameServer)
{
	m_GameFlags = 0;
	m_SelfDamage = false;
}

CGameControllerBaseInstagib::~CGameControllerBaseInstagib() = default;

bool CGameControllerBaseInstagib::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	if(Dmg < g_Config.m_SvDamageNeededForKill && Weapon == WEAPON_GRENADE)
		return true;

	return CGameControllerBasePvp::SkipDamage(Dmg, From, Weapon, pCharacter, ApplyForce);
}

void CGameControllerBaseInstagib::OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	Dmg = 20;
	CGameControllerBasePvp::OnAppliedDamage(Dmg, From, Weapon, pCharacter);
}

bool CGameControllerBaseInstagib::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	if(IsPickupEntity(Index))
		return false;

	return CGameControllerBasePvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}
