/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>

#include <game/server/entities/character.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/player.h>

#include "pvp_projectile.h"

CPvpProjectile::CPvpProjectile(
	CGameWorld *pGameWorld,
	int Type,
	int Owner,
	vec2 Pos,
	vec2 Dir,
	int Span,
	bool Freeze,
	bool Explosive,
	int SoundImpact,
	vec2 InitDir,
	int Layer,
	int Number) :
	CProjectile(
		pGameWorld,
		Type,
		Owner,
		Pos,
		Dir,
		Span,
		Freeze,
		Explosive,
		SoundImpact,
		InitDir,
		Layer,
		Number)
{
	m_FirstTick = true;
	m_OrigStartTick = m_StartTick;
	m_FirstSnap = true;

	for(int &ParticleId : m_aParticleIds)
	{
		ParticleId = Server()->SnapNewId();
	}
}

CPvpProjectile::~CPvpProjectile()
{
	for(int ParticleId : m_aParticleIds)
	{
		Server()->SnapFreeId(ParticleId);
	}
}

void CPvpProjectile::Tick()
{
	bool IsRollbackDamage = false; //ddnet-insta
	int RollbackDamageTick = 0; //ddnet-insta

	int Collide = 0;
	CCharacter *pTargetChr = 0;
	float Pt;
	float Ct;
	vec2 PrevPos;
	vec2 CurPos;
	vec2 ColPos;
	vec2 NewPos;

	CCharacter *pOwnerChar = 0;

	bool IsWeaponCollide = false;

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(m_FirstTick && m_Owner >= 0 && m_Owner < MAX_CLIENTS && GameServer()->m_apPlayers[m_Owner] && GameServer()->m_apPlayers[m_Owner]->m_RollbackEnabled && GameServer()->m_apPlayers[m_Owner]->GetCharacter())
	{
		m_StartTick = GameServer()->m_apPlayers[m_Owner]->m_LastAckedSnapshot + 1;

		//Collide with wall and tee
		int CollideTick;
		for(CollideTick = m_StartTick + 1; CollideTick <= m_OrigStartTick; CollideTick++)
		{
			Pt = (CollideTick - m_StartTick - 1) / (float)Server()->TickSpeed();
			Ct = (CollideTick - m_StartTick) / (float)Server()->TickSpeed();
			PrevPos = GetPos(Pt);
			CurPos = GetPos(Ct);
			Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos); //wall

			if(m_LifeSpan > -1)
				m_LifeSpan--;

			if(Collide)
				break;

			pTargetChr = GameServer()->m_Rollback.IntersectCharacterOnTick(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, ColPos, pOwnerChar, m_Owner, nullptr, CollideTick); //tee

			if(pTargetChr)
			{
				IsWeaponCollide = false; //just explode >:(
				IsRollbackDamage = true;
				RollbackDamageTick = CollideTick;
				break;
			}

			if(Collide)
				break;

			if(m_LifeSpan == -1)
				break;
		}
	}
	else
	{
		Pt = (Server()->Tick() - m_StartTick - 1) / (float)Server()->TickSpeed();
		Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();
		PrevPos = GetPos(Pt);
		CurPos = GetPos(Ct);
		if(!Collide)
			Collide = GameServer()->Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos);

		if(!pTargetChr && (pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit))
			pTargetChr = GameServer()->m_World.IntersectCharacter(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, ColPos, pOwnerChar, m_Owner);
	}

	if(m_LifeSpan > -1)
		m_LifeSpan--;

	CClientMask TeamMask = CClientMask().set();
	//bool IsWeaponCollide = false;
	if(
		pOwnerChar &&
		pTargetChr &&
		pOwnerChar->IsAlive() &&
		pTargetChr->IsAlive() &&
		!pTargetChr->CanCollide(m_Owner))
	{
		IsWeaponCollide = true;
	}
	if(pOwnerChar && pOwnerChar->IsAlive())
	{
		TeamMask = pOwnerChar->TeamMask();
	}
	else if(m_Owner >= 0 && (m_Type != WEAPON_GRENADE || g_Config.m_SvDestroyBulletsOnDeath || m_BelongsToPracticeTeam))
	{
		m_MarkedForDestroy = true;
		return;
	}

	if(((pTargetChr && (pOwnerChar ? !pOwnerChar->GrenadeHitDisabled() : g_Config.m_SvHit || m_Owner == -1 || pTargetChr == pOwnerChar)) || Collide || GameLayerClipped(CurPos)) && !IsWeaponCollide)
	{
		if(m_Explosive /*??*/ && (!pTargetChr || (pTargetChr && (!m_Freeze || (m_Type == WEAPON_SHOTGUN && Collide)))))
		{
			int Number = 1;
			if(GameServer()->EmulateBug(BUG_GRENADE_DOUBLEEXPLOSION) && m_LifeSpan == -1)
			{
				Number = 2;
			}
			for(int i = 0; i < Number; i++)
			{
				if(IsRollbackDamage)
				{
					GameServer()->m_Rollback.CreateExplosionOnTick(ColPos, m_Owner, m_Type, m_Owner == -1, (!pTargetChr ? -1 : pTargetChr->Team()),
						RollbackDamageTick, (m_Owner != -1) ? TeamMask : CClientMask().set());
				}
				else
				{
					GameServer()->CreateExplosion(ColPos, m_Owner, m_Type, m_Owner == -1, (!pTargetChr ? -1 : pTargetChr->Team()),
						(m_Owner != -1) ? TeamMask : CClientMask().set());
				}
				GameServer()->CreateSound(ColPos, m_SoundImpact,
					(m_Owner != -1) ? TeamMask : CClientMask().set());
			}
		}
		else if(m_Freeze)
		{
			CEntity *apEnts[MAX_CLIENTS];
			int Num = GameWorld()->FindEntities(CurPos, 1.0f, apEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);
			for(int i = 0; i < Num; ++i)
			{
				auto *pChr = static_cast<CCharacter *>(apEnts[i]);
				if(pChr && (m_Layer != LAYER_SWITCH || (m_Layer == LAYER_SWITCH && m_Number > 0 && Switchers()[m_Number].m_aStatus[pChr->Team()])))
					pChr->Freeze();
			}
		}
		else if(pTargetChr) // ddnet-insta
			pTargetChr->TakeDamage(vec2(0, 0), 0, m_Owner, m_Type);

		if(pOwnerChar && !GameLayerClipped(ColPos) &&
			((m_Type == WEAPON_GRENADE && pOwnerChar->HasTelegunGrenade()) || (m_Type == WEAPON_GUN && pOwnerChar->HasTelegunGun())))
		{
			int MapIndex = GameServer()->Collision()->GetPureMapIndex(pTargetChr ? pTargetChr->m_Pos : ColPos);
			int TileFIndex = GameServer()->Collision()->GetFrontTileIndex(MapIndex);
			bool IsSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_TELE_GUN;
			bool IsBlueSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_BLUE_TELE_GUN;

			if(IsSwitchTeleGun || IsBlueSwitchTeleGun)
			{
				// Delay specifies which weapon the tile should work for.
				// Delay = 0 means all.
				int delay = GameServer()->Collision()->GetSwitchDelay(MapIndex);

				if(delay == 1 && m_Type != WEAPON_GUN)
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
				if(delay == 2 && m_Type != WEAPON_GRENADE)
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
				if(delay == 3 && m_Type != WEAPON_LASER)
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
			}

			if(TileFIndex == TILE_ALLOW_TELE_GUN || TileFIndex == TILE_ALLOW_BLUE_TELE_GUN || IsSwitchTeleGun || IsBlueSwitchTeleGun || pTargetChr)
			{
				bool Found;
				vec2 PossiblePos;

				if(!Collide)
					Found = GetNearestAirPosPlayer(pTargetChr ? pTargetChr->m_Pos : ColPos, &PossiblePos);
				else
					Found = GetNearestAirPos(NewPos, CurPos, &PossiblePos);

				if(Found)
				{
					pOwnerChar->m_TeleGunPos = PossiblePos;
					pOwnerChar->m_TeleGunTeleport = true;
					pOwnerChar->m_IsBlueTeleGunTeleport = TileFIndex == TILE_ALLOW_BLUE_TELE_GUN || IsBlueSwitchTeleGun;
				}
			}
		}

		if(Collide && m_Bouncing != 0)
		{
			m_StartTick = Server()->Tick();
			m_Pos = NewPos + (-(m_Direction * 4));
			if(m_Bouncing == 1)
				m_Direction.x = -m_Direction.x;
			else if(m_Bouncing == 2)
				m_Direction.y = -m_Direction.y;
			if(absolute(m_Direction.x) < 1e-6f)
				m_Direction.x = 0;
			if(absolute(m_Direction.y) < 1e-6f)
				m_Direction.y = 0;
			m_Pos += m_Direction;
		}
		else if(m_Type == WEAPON_GUN)
		{
			// ddnet-insta commeted out dmg indicators
			// GameServer()->CreateDamageInd(CurPos, -std::atan2(m_Direction.x, m_Direction.y), 10, (m_Owner != -1) ? TeamMask : CClientMask().set());
			m_MarkedForDestroy = true;
			return;
		}
		else
		{
			if(!m_Freeze)
			{
				m_MarkedForDestroy = true;
				return;
			}
		}
	}
	if(m_LifeSpan == -1)
	{
		if(m_Explosive)
		{
			if(m_Owner >= 0)
				pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

			TeamMask = CClientMask().set();
			if(pOwnerChar && pOwnerChar->IsAlive())
			{
				TeamMask = pOwnerChar->TeamMask();
			}

			GameServer()->CreateExplosion(ColPos, m_Owner, m_Type, m_Owner == -1, (!pOwnerChar ? -1 : pOwnerChar->Team()),
				(m_Owner != -1) ? TeamMask : CClientMask().set());
			GameServer()->CreateSound(ColPos, m_SoundImpact,
				(m_Owner != -1) ? TeamMask : CClientMask().set());
		}
		m_MarkedForDestroy = true;
		return;
	}

	int x = GameServer()->Collision()->GetIndex(PrevPos, CurPos);
	int z;
	if(g_Config.m_SvOldTeleportWeapons)
		z = GameServer()->Collision()->IsTeleport(x);
	else
		z = GameServer()->Collision()->IsTeleportWeapon(x);
	if(z && !GameServer()->Collision()->TeleOuts(z - 1).empty())
	{
		int TeleOut = GameServer()->m_World.m_Core.RandomOr0(GameServer()->Collision()->TeleOuts(z - 1).size());
		m_Pos = GameServer()->Collision()->TeleOuts(z - 1)[TeleOut];
		m_StartTick = Server()->Tick();
	}

	m_FirstTick = false;
}

void CPvpProjectile::Snap(int SnappingClient)
{
	float Ct = (Server()->Tick() - m_StartTick) / (float)Server()->TickSpeed();

	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	//Kaizo-Insta projectile rollback particles
	if(m_FirstSnap && m_Owner >= 0 && m_Owner < MAX_CLIENTS && GameServer()->m_apPlayers[m_Owner] && GameServer()->m_apPlayers[m_Owner]->m_RollbackEnabled)
	{
		for(int i = 0; i < 3; i++)
		{
			{
				CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(m_aParticleIds[i]);
				if(!pProj)
				{
					continue;
				}
				pProj->m_X = GetPos((Server()->Tick() - (m_OrigStartTick - (i * 2 + 3))) / (float)Server()->TickSpeed()).x;
				pProj->m_Y = GetPos((Server()->Tick() - (m_OrigStartTick - (i * 2 + 3))) / (float)Server()->TickSpeed()).y;
				pProj->m_VelX = 0;
				pProj->m_VelY = 0;
				pProj->m_StartTick = Server()->Tick();
				pProj->m_Type = WEAPON_HAMMER;
			}
		}
	}

	// ddnet-insta
	if(m_Type == WEAPON_SHOTGUN)
	{
		CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetId(), sizeof(CNetObj_Projectile)));
		if(pProj)
			FillInfo(pProj);
		return;
	}

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	if(SnappingClientVersion < VERSION_DDNET_ENTITY_NETOBJS)
	{
		CCharacter *pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
		int Tick = (Server()->Tick() % Server()->TickSpeed()) % ((m_Explosive) ? 6 : 20);
		if(pSnapChar && pSnapChar->IsAlive() && (m_Layer == LAYER_SWITCH && m_Number > 0 && !Switchers()[m_Number].m_aStatus[pSnapChar->Team()] && (!Tick)))
			return;
	}

	CCharacter *pOwnerChar = 0;
	CClientMask TeamMask = CClientMask().set();

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(pOwnerChar && pOwnerChar->IsAlive())
		TeamMask = pOwnerChar->TeamMask();

	if(SnappingClient != SERVER_DEMO_CLIENT && m_Owner != -1 && !TeamMask.test(SnappingClient))
		return;

	CNetObj_DDRaceProjectile DDRaceProjectile;

	if(SnappingClientVersion >= VERSION_DDNET_ENTITY_NETOBJS)
	{
		CNetObj_DDNetProjectile *pDDNetProjectile = static_cast<CNetObj_DDNetProjectile *>(Server()->SnapNewItem(NETOBJTYPE_DDNETPROJECTILE, GetId(), sizeof(CNetObj_DDNetProjectile)));
		if(!pDDNetProjectile)
		{
			return;
		}
		FillExtraInfo(pDDNetProjectile);
		if(m_Owner == SnappingClient) //Kaizo-Insta rollback antiping workaround
			pDDNetProjectile->m_StartTick = m_OrigStartTick;
	}
	else if(SnappingClientVersion >= VERSION_DDNET_ANTIPING_PROJECTILE && FillExtraInfoLegacy(&DDRaceProjectile))
	{
		int Type = SnappingClientVersion < VERSION_DDNET_MSG_LEGACY ? (int)NETOBJTYPE_PROJECTILE : NETOBJTYPE_DDRACEPROJECTILE;
		void *pProj = Server()->SnapNewItem(Type, GetId(), sizeof(DDRaceProjectile));
		if(!pProj)
		{
			return;
		}
		if(m_Owner == SnappingClient) //Kaizo-Insta rollback antiping workaround
			DDRaceProjectile.m_StartTick = m_OrigStartTick;
		mem_copy(pProj, &DDRaceProjectile, sizeof(DDRaceProjectile));
	}
	else
	{
		CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(GetId());
		if(!pProj)
		{
			return;
		}
		FillInfo(pProj);
	}

	m_FirstSnap = false;
}
