#include "base_instagib.h"

#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

CGameControllerInstagib::CGameControllerInstagib(class CGameContext *pGameServer) :
	CGameControllerBasePvp(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
	m_IsVanillaGameType = false;
	m_SelfDamage = false;
}

CGameControllerInstagib::~CGameControllerInstagib() = default;

bool CGameControllerInstagib::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	if(Dmg < g_Config.m_SvDamageNeededForKill && Weapon == WEAPON_GRENADE)
		return true;

	return CGameControllerBasePvp::SkipDamage(Dmg, From, Weapon, pCharacter, ApplyForce);
}

void CGameControllerInstagib::OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	Dmg = 20;
	CGameControllerBasePvp::OnAppliedDamage(Dmg, From, Weapon, pCharacter);
}

bool CGameControllerInstagib::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	if(IsPickupEntity(Index))
		return false;

	return CGameControllerBasePvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}
