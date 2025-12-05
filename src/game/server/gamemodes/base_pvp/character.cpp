#include "base_pvp.h"

#include <base/log.h>

#include <engine/antibot.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>
#include <generated/server_data.h>

#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

bool CCharacter::IsTouchingTile(int Tile)
{
	if(!Collision()->GameLayer())
		return false;

	float Prox = GetProximityRadius() / 3.f;
	int Left = (m_Pos.x - Prox) / 32;
	int Right = (m_Pos.x + Prox) / 32;
	int Up = (m_Pos.y - Prox) / 32;
	int Down = (m_Pos.y + Prox) / 32;

	if((Collision()->GetIndex(Right, Up) == Tile ||
		   Collision()->GetIndex(Right, Down) == Tile ||
		   Collision()->GetIndex(Left, Up) == Tile ||
		   Collision()->GetIndex(Left, Down) == Tile))
	{
		return true;
	}

	if(!Collision()->FrontLayer())
		return false;

	if((Collision()->GetFrontIndex(Right, Up) == Tile ||
		   Collision()->GetFrontIndex(Right, Down) == Tile ||
		   Collision()->GetFrontIndex(Left, Up) == Tile ||
		   Collision()->GetFrontIndex(Left, Down) == Tile))
	{
		return true;
	}
	return false;
}

float CCharacter::DistToTouchingTile(int Tile)
{
#define NOT_FOUND 1024.0f

	if(!Collision()->GameLayer())
		return NOT_FOUND;

	float Prox = GetProximityRadius() / 3.f;
	int Left = std::clamp(round_to_int(m_Pos.x - Prox) / 32, 0, Collision()->GetWidth() - 1);
	int Right = std::clamp(round_to_int(m_Pos.x + Prox) / 32, 0, Collision()->GetWidth() - 1);
	int Up = std::clamp(round_to_int(m_Pos.y - Prox) / 32, 0, Collision()->GetHeight() - 1);
	int Down = std::clamp(round_to_int(m_Pos.y + Prox) / 32, 0, Collision()->GetHeight() - 1);

	vec2 TeeCenter;
	TeeCenter.x = std::clamp(round_to_int(m_Pos.x), 0, (Collision()->GetWidth() - 1) * 32);
	TeeCenter.y = std::clamp(round_to_int(m_Pos.y), 0, (Collision()->GetHeight() - 1) * 32);

	int aPositionsX[] = {Left, Right};
	int aPositionsY[] = {Up, Down};

	float ClosestDistance = NOT_FOUND;

	for(int PosX : aPositionsX)
	{
		for(int PosY : aPositionsY)
		{
			if((Collision()->GetIndex(PosX, PosY) == Tile))
			{
				vec2 TileCenter = vec2(PosX, PosY);
				TileCenter.x *= 32;
				TileCenter.y *= 32;
				TileCenter.x += 16;
				TileCenter.y += 16;
				float Dist = distance(TileCenter, TeeCenter);
				if(Dist < ClosestDistance)
					ClosestDistance = Dist;
			}
		}
	}

	if(!Collision()->FrontLayer())
		return ClosestDistance;

	for(int PosX : aPositionsX)
	{
		for(int PosY : aPositionsY)
		{
			if((Collision()->GetFrontIndex(PosX, PosY) == Tile))
			{
				vec2 TileCenter = vec2(PosX / 32, PosY / 32);
				TileCenter.x += 16;
				TileCenter.y += 16;
				float Dist = distance(TileCenter, TeeCenter);
				if(Dist < ClosestDistance)
					ClosestDistance = Dist;
			}
		}
	}

#undef NOT_FOUND
	return ClosestDistance;
}

void CCharacter::AmmoRegen()
{
	// ammo regen on Grenade
	int AmmoRegenTime = 0;
	int MaxAmmo = 0;
	if(m_Core.m_ActiveWeapon == WEAPON_GRENADE && g_Config.m_SvGrenadeAmmoRegen)
	{
		AmmoRegenTime = g_Config.m_SvGrenadeAmmoRegenTime;
		MaxAmmo = g_Config.m_SvGrenadeAmmoRegenNum;
	}
	else
	{
		// ammo regen
		AmmoRegenTime = g_pData->m_Weapons.m_aId[m_Core.m_ActiveWeapon].m_Ammoregentime;
		MaxAmmo = g_pData->m_Weapons.m_aId[m_Core.m_ActiveWeapon].m_Maxammo;
	}
	if(AmmoRegenTime && m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo >= 0)
	{
		// If equipped and not active, regen ammo?
		if(m_ReloadTimer <= 0)
		{
			if(m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if((Server()->Tick() - m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo = minimum(m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_Ammo + 1, MaxAmmo);
				m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}
}

void CCharacter::ResetInstaSettings()
{
	if(m_Core.m_aWeapons[WEAPON_GRENADE].m_Got)
	{
		// TODO: IsVanillaGameType() is probably not the best check for infinite ammo mode
		//       see also the todo below for a possible refactor
		bool IsInfiniteAmmoMode = GameServer()->m_pController && GameServer()->m_pController->IsVanillaGameType();

		// TODO: add DefaultAmmoGrenade(), DefaultAmmoShotgun(), ...
		//       to the controller and call it here
		//       so that gametypes can define their own ammo defaults
		//       and it will be reset correctly
		int Ammo = IsInfiniteAmmoMode ? -1 : 10;
		if(g_Config.m_SvGrenadeAmmoRegen)
			Ammo = g_Config.m_SvGrenadeAmmoRegenNum;
		m_Core.m_aWeapons[WEAPON_GRENADE].m_AmmoRegenStart = -1;
		m_Core.m_aWeapons[WEAPON_GRENADE].m_Ammo = Ammo;
	}
}

void CCharacter::Rainbow(bool Activate)
{
	if(Activate == m_Rainbow)
		return;

	m_Rainbow = Activate;

	if(Activate)
		GetPlayer()->m_SkinInfoManager.SetUseCustomColor(ESkinPrio::RAINBOW, true);
	else
		GetPlayer()->m_SkinInfoManager.UnsetAll(ESkinPrio::RAINBOW);
}

void CCharacter::PlayerGetBall()
{
	if(m_LoseBallTick)
		return;

	GiveWeapon(WEAPON_GRENADE, false, 1);
	SetWeapon(WEAPON_GRENADE);
	m_LoseBallTick = Server()->Tick() + Server()->TickSpeed() * 3;
}

void CCharacter::LoseBall()
{
	GiveWeapon(WEAPON_GRENADE, true);
	SetWeapon(WEAPON_HAMMER);
	m_LoseBallTick = 0;
}
