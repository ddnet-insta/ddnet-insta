#include <base/system.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamemodes/base_pvp/base_pvp.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include "base_instagib.h"

CGameControllerInstagib::CGameControllerInstagib(class CGameContext *pGameServer) :
	CGameControllerPvp(pGameServer)
{
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
	m_IsVanillaGameType = false;
	m_SelfDamage = false;
	m_AllowSkinColorChange = true;
}

CGameControllerInstagib::~CGameControllerInstagib() = default;

bool CGameControllerInstagib::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	if(Dmg < g_Config.m_SvDamageNeededForKill && Weapon == WEAPON_GRENADE)
		return true;

	return CGameControllerPvp::SkipDamage(Dmg, From, Weapon, pCharacter, ApplyForce);
}

void CGameControllerInstagib::OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	Dmg = 20;
	CGameControllerPvp::OnAppliedDamage(Dmg, From, Weapon, pCharacter);
}

bool CGameControllerInstagib::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	// ddnet-insta
	// do not spawn pickups
	if(Index == ENTITY_ARMOR_1 || Index == ENTITY_HEALTH_1 || Index == ENTITY_WEAPON_SHOTGUN || Index == ENTITY_WEAPON_GRENADE || Index == ENTITY_WEAPON_LASER || Index == ENTITY_POWERUP_NINJA)
		return false;

	return CGameControllerPvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}
