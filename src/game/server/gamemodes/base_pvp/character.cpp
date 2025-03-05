#include <engine/antibot.h>
#include <engine/shared/config.h>
#include <game/generated/server_data.h>
#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include "base_pvp.h"

void CGameControllerPvp::OnCharacterConstruct(class CCharacter *pChr)
{
	pChr->m_IsGodmode = false;
}

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
	int Left = clamp(round_to_int(m_Pos.x - Prox) / 32, 0, Collision()->GetWidth() - 1);
	int Right = clamp(round_to_int(m_Pos.x + Prox) / 32, 0, Collision()->GetWidth() - 1);
	int Up = clamp(round_to_int(m_Pos.y - Prox) / 32, 0, Collision()->GetHeight() - 1);
	int Down = clamp(round_to_int(m_Pos.y + Prox) / 32, 0, Collision()->GetHeight() - 1);

	vec2 TeeCenter;
	TeeCenter.x = clamp(round_to_int(m_Pos.x), 0, (Collision()->GetWidth() - 1) * 32);
	TeeCenter.y = clamp(round_to_int(m_Pos.y), 0, (Collision()->GetHeight() - 1) * 32);

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

void CCharacter::TakeHammerHit(CCharacter *pFrom, vec2 &Force)
{
	if(g_Config.m_SvFngHammer)
	{
		// TODO: can we get this value from Force without recomputing it?
		vec2 Dir;
		if(length(m_Pos - pFrom->m_Pos) > 0.0f)
			Dir = normalize(m_Pos - pFrom->m_Pos);
		else
			Dir = vec2(0.f, -1.f);

		vec2 Push = vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;

		// matches ddnet clients prediction code by default
		// https://github.com/ddnet/ddnet/blob/f9df4a85be4ca94ca91057cd447707bcce16fd94/src/game/client/prediction/entities/character.cpp#L334-L346
		if(GameServer()->m_pController->IsTeamPlay() && pFrom->GetPlayer() && m_pPlayer->GetTeam() == pFrom->GetPlayer()->GetTeam() && m_FreezeTime)
		{
			Push.x *= g_Config.m_SvMeltHammerScaleX * 0.01f;
			Push.y *= g_Config.m_SvMeltHammerScaleY * 0.01f;
		}
		else
		{
			Push.x *= g_Config.m_SvHammerScaleX * 0.01f;
			Push.y *= g_Config.m_SvHammerScaleY * 0.01f;
		}

		Force = Push;
	}

	CPlayer *pPlayer = pFrom->GetPlayer();

	// this should really never happend but is needed to calm down clang
	if(!pPlayer)
		return;

	// TODO: this should be refactored into a descriptive method
	//       called fng unmelt hammer or something like that
	//       and it should also be behind a config so that all modes could use it
	//       i can also see this being fun in ddrace ctf
	if(GameServer()->m_pController->IsFngGameType())
	{
		if(GameServer()->m_pController->IsTeamPlay() && pPlayer->GetTeam() == m_pPlayer->GetTeam())
		{
			if(m_FreezeTime)
			{
				m_FreezeTime -= Server()->TickSpeed() * 3;

				// make sure we don't got negative and let the ddrace tick trigger the unfreeeze
				if(m_FreezeTime < 2)
				{
					m_FreezeTime = 2;

					// reward the unfreezer with one point
					pPlayer->AddScore(1);
					if(GameServer()->m_pController->IsStatTrack())
						pPlayer->m_Stats.m_Unfreezes++;
				}
			}
		}
	}
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
	int Ammo = -1;
	// TODO: this should not check the default weapon
	//       but if any of the spawn weapons is a grenade
	if(GameServer()->m_pController->GetDefaultWeapon(GetPlayer()) == WEAPON_GRENADE)
	{
		Ammo = g_Config.m_SvGrenadeAmmoRegen ? g_Config.m_SvGrenadeAmmoRegenNum : -1;
		m_Core.m_aWeapons[m_Core.m_ActiveWeapon].m_AmmoRegenStart = -1;
	}
	GiveWeapon(GameServer()->m_pController->GetDefaultWeapon(GetPlayer()), false, Ammo);
}

void CCharacter::Rainbow(bool Activate)
{
	if(Activate == m_Rainbow)
		return;

	m_Rainbow = Activate;

	if(Activate)
	{
		GetPlayer()->m_TeeInfosNoCosmetics = GetPlayer()->m_TeeInfos;
		return;
	}

	GetPlayer()->m_TeeInfos = GetPlayer()->m_TeeInfosNoCosmetics;
	GetPlayer()->m_TeeInfos.ToSixup();

	protocol7::CNetMsg_Sv_SkinChange Msg;
	Msg.m_ClientId = GetPlayer()->GetCid();
	for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
	{
		Msg.m_apSkinPartNames[p] = GetPlayer()->m_TeeInfos.m_apSkinPartNames[p];
		Msg.m_aSkinPartColors[p] = GetPlayer()->m_TeeInfos.m_aSkinPartColors[p];
		Msg.m_aUseCustomColors[p] = GetPlayer()->m_TeeInfos.m_aUseCustomColors[p];
	}

	for(CPlayer *pRainbowReceiverPlayer : GameServer()->m_apPlayers)
	{
		if(!pRainbowReceiverPlayer)
			continue;
		if(!Server()->IsSixup(pRainbowReceiverPlayer->GetCid()))
			continue;

		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, pRainbowReceiverPlayer->GetCid());
	}
}
