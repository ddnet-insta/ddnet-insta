#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/ddnet_pvp/vanilla_pickup.h>
#include <game/server/player.h>

#include "ctf.h"

CGameControllerCTF::CGameControllerCTF(class CGameContext *pGameServer) :
	CGameControllerBaseCTF(pGameServer)
{
	m_pGameType = "CTF*";
	m_IsVanillaGameType = true;
	m_GameFlags = GAMEFLAG_TEAMS | GAMEFLAG_FLAGS;
	m_AllowSkinColorChange = true;
	m_DefaultWeapon = WEAPON_GUN;

	m_pStatsTable = "ctf";
	m_pExtraColumns = new CCtfColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerCTF::~CGameControllerCTF() = default;

void CGameControllerCTF::Tick()
{
	CGameControllerBaseCTF::Tick();
}

void CGameControllerCTF::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBaseCTF::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

int CGameControllerCTF::SnapGameInfoExFlags(int SnappingClient, int DDRaceFlags)
{
	int Flags = CGameControllerPvp::SnapGameInfoExFlags(SnappingClient, DDRaceFlags);
	Flags &= ~(GAMEINFOFLAG_UNLIMITED_AMMO);
	Flags &= ~(GAMEINFOFLAG_PREDICT_DDRACE);
	Flags &= ~(GAMEINFOFLAG_PREDICT_DDRACE_TILES); // https://github.com/ddnet-insta/ddnet-insta/issues/181
	return Flags;
}

bool CGameControllerCTF::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
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

bool CGameControllerCTF::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	const vec2 Pos(x * 32.0f + 16.0f, y * 32.0f + 16.0f);

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

	return CGameControllerBaseCTF::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}
