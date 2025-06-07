#include <base/math.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/ddnet_pvp/vanilla_pickup.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include "base_vanilla.h"

CGameControllerVanilla::CGameControllerVanilla(class CGameContext *pGameServer) :
	CGameControllerPvp(pGameServer)
{
	m_GameFlags = 0;
	m_IsVanillaGameType = true;
	m_AllowSkinColorChange = true;
	m_DefaultWeapon = WEAPON_GUN;
}

CGameControllerVanilla::~CGameControllerVanilla() = default;

int CGameControllerVanilla::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
{
	int Flags = CGameControllerPvp::SnapGameInfoExFlags(SnappingClient, DDRaceFlags);
	Flags &= ~(GAMEINFOFLAG_UNLIMITED_AMMO);
	// Flags &= ~(GAMEINFOFLAG_PREDICT_DDRACE); // https://github.com/ddnet-insta/ddnet-insta/issues/120
	Flags &= ~(GAMEINFOFLAG_PREDICT_DDRACE_TILES); // https://github.com/ddnet-insta/ddnet-insta/issues/181
	return Flags;
}

bool CGameControllerVanilla::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	if(Weapon == WEAPON_GUN || Weapon == WEAPON_SHOTGUN)
		Dmg = 1;
	if(Weapon == WEAPON_LASER)
		Dmg = 5;
	// this would mess with explosion damage
	// https://github.com/ddnet-insta/ddnet-insta/issues/135
	// if(Weapon == WEAPON_GRENADE)
	// 	Dmg = 6;

	OnAnyDamage(Force, Dmg, From, Weapon, &Character);
	bool ApplyForce = true;
	if(SkipDamage(Dmg, From, Weapon, &Character, ApplyForce))
	{
		Dmg = 0;
		return !ApplyForce;
	}
	OnAppliedDamage(Dmg, From, Weapon, &Character);
	ApplyVanillaDamage(Dmg, From, Weapon, &Character);
	DecreaseHealthAndKill(Dmg, From, Weapon, &Character);
	return false;
}

bool CGameControllerVanilla::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	const vec2 Pos((x * 32.0f) + 16.0f, (y * 32.0f) + 16.0f);

	int Type = -1;
	int SubType = 0;

	if(Index == ENTITY_ARMOR_1)
		Type = POWERUP_ARMOR;
	else if(Index == ENTITY_ARMOR_SHOTGUN)
		Type = POWERUP_ARMOR_SHOTGUN;
	else if(Index == ENTITY_ARMOR_GRENADE)
		Type = POWERUP_ARMOR_GRENADE;
	else if(Index == ENTITY_ARMOR_NINJA)
		Type = POWERUP_ARMOR_NINJA;
	else if(Index == ENTITY_ARMOR_LASER)
		Type = POWERUP_ARMOR_LASER;
	else if(Index == ENTITY_HEALTH_1)
		Type = POWERUP_HEALTH;
	else if(Index == ENTITY_WEAPON_SHOTGUN)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_SHOTGUN;
	}
	else if(Index == ENTITY_WEAPON_GRENADE)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_GRENADE;
	}
	else if(Index == ENTITY_WEAPON_LASER)
	{
		Type = POWERUP_WEAPON;
		SubType = WEAPON_LASER;
	}
	else if(Index == ENTITY_POWERUP_NINJA)
	{
		if(!g_Config.m_SvPowerups)
			return true;

		Type = POWERUP_NINJA;
		SubType = WEAPON_NINJA;
	}

	if(Type != -1) // NOLINT(clang-analyzer-unix.Malloc)
	{
		CVanillaPickup *pPickup = new CVanillaPickup(&GameServer()->m_World, Type, SubType, Layer, Number);
		pPickup->m_Pos = Pos;
		return true; // NOLINT(clang-analyzer-unix.Malloc)
	}

	return CGameControllerPvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}
