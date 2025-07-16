/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "pvp_laser.h"
#include <game/server/entities/character.h>

#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>

#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>
#include <game/server/player.h>

#include <game/server/instagib/rollback.h>

CPvpLaser::CPvpLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Type) :
	CLaser(pGameWorld)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_TelePos = vec2(0, 0);
	m_WasTele = false;
	m_Type = Type;
	m_TeleportCancelled = false;
	m_IsBlueTeleport = false;
	m_ZeroEnergyBounceInLastTick = false;
	m_TuneZone = GameServer()->Collision()->IsTune(GameServer()->Collision()->GetMapIndex(m_Pos));
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	m_TeamMask = pOwnerChar ? pOwnerChar->TeamMask() : CClientMask();
	m_BelongsToPracticeTeam = pOwnerChar && pOwnerChar->Teams()->IsPractice(pOwnerChar->Team());

	GameWorld()->InsertEntity(this);
	DoBouncePvp();
}

bool CPvpLaser::HitCharacterPvp(vec2 From, vec2 To)
{
	static const vec2 StackedLaserShotgunBugSpeed = vec2(-2147483648.0f, -2147483648.0f);
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit;
	bool pDontHitSelf = g_Config.m_SvOldLaser || (m_Bounces == 0 && !m_WasTele);

	if(pOwnerChar ? (!pOwnerChar->LaserHitDisabled() && m_Type == WEAPON_LASER) || (!pOwnerChar->ShotgunHitDisabled() && m_Type == WEAPON_SHOTGUN) : g_Config.m_SvHit)
		pHit = GameServer()->m_Rollback.IntersectCharacterOnTick(m_Pos, To, 0.f, At, pDontHitSelf ? pOwnerChar : nullptr, m_Owner, nullptr, m_FireAckedTick);
	else
		pHit = GameServer()->m_Rollback.IntersectCharacterOnTick(m_Pos, To, 0.f, At, pDontHitSelf ? pOwnerChar : nullptr, m_Owner, pOwnerChar, m_FireAckedTick);

	if(!pHit || (pHit == pOwnerChar && g_Config.m_SvOldLaser) || (pHit != pOwnerChar && pOwnerChar ? (pOwnerChar->LaserHitDisabled() && m_Type == WEAPON_LASER) || (pOwnerChar->ShotgunHitDisabled() && m_Type == WEAPON_SHOTGUN) : !g_Config.m_SvHit))
		return false;
	m_From = From;
	m_Pos = At;
	m_Energy = -1;
	if(m_Type == WEAPON_SHOTGUN)
	{
		float Strength;
		if(!m_TuneZone)
			Strength = Tuning()->m_ShotgunStrength;
		else
			Strength = TuningList()[m_TuneZone].m_ShotgunStrength;

		const vec2 &HitPos = pHit->Core()->m_Pos;
		if(!g_Config.m_SvOldLaser)
		{
			if(m_PrevPos != HitPos)
			{
				pHit->AddVelocity(normalize(m_PrevPos - HitPos) * Strength);
			}
			else
			{
				pHit->SetRawVelocity(StackedLaserShotgunBugSpeed);
			}
		}
		else if(g_Config.m_SvOldLaser && pOwnerChar)
		{
			if(pOwnerChar->Core()->m_Pos != HitPos)
			{
				pHit->AddVelocity(normalize(pOwnerChar->Core()->m_Pos - HitPos) * Strength);
			}
			else
			{
				pHit->SetRawVelocity(StackedLaserShotgunBugSpeed);
			}
		}
		else
		{
			// Re-apply move restrictions as a part of 'shotgun bug' reproduction
			pHit->ApplyMoveRestrictions();
		}
	}
	else if(m_Type == WEAPON_LASER)
	{
		// ddnet-insta fng
		// pHit->UnFreeze();
	}
	if(GameServer()->m_pController->OnLaserHit(m_Bounces, m_Owner, m_Type, pHit))
		pHit->TakeDamage(vec2(0, 0), 0, m_Owner, m_Type);
	return true;
}

void CPvpLaser::DoBouncePvp()
{
	if(GameServer()->m_apPlayers[m_Owner] && GameServer()->m_apPlayers[m_Owner]->m_RollbackEnabled)
	{
		if(m_FireAckedTick == -1)
		{
			m_FireAckedTick = GameServer()->m_apPlayers[m_Owner]->m_LastAckedSnapshot;
		}
		else
		{
			m_FireAckedTick += Server()->Tick() - m_EvalTick; //rollback (ddnet-insta specific): increment acked tick each bounce
		}
	}

	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		m_MarkedForDestroy = true;
		return;
	}
	m_PrevPos = m_Pos;
	vec2 Coltile;

	int Res;
	int z;

	if(m_WasTele)
	{
		m_PrevPos = m_TelePos;
		m_Pos = m_TelePos;
		m_TelePos = vec2(0, 0);
	}

	vec2 To = m_Pos + m_Dir * m_Energy;

	Res = GameServer()->Collision()->IntersectLineTeleWeapon(m_Pos, To, &Coltile, &To, &z);

	if(Res)
	{
		if(!HitCharacterPvp(m_Pos, To))
		{
			// intersected
			m_From = m_Pos;
			m_Pos = To;

			vec2 TempPos = m_Pos;
			vec2 TempDir = m_Dir * 4.0f;

			int f = 0;
			if(Res == -1)
			{
				f = GameServer()->Collision()->GetTile(round_to_int(Coltile.x), round_to_int(Coltile.y));
				GameServer()->Collision()->SetCollisionAt(round_to_int(Coltile.x), round_to_int(Coltile.y), TILE_SOLID);
			}
			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, nullptr);
			if(Res == -1)
			{
				GameServer()->Collision()->SetCollisionAt(round_to_int(Coltile.x), round_to_int(Coltile.y), f);
			}
			m_Pos = TempPos;
			m_Dir = normalize(TempDir);

			const float Distance = distance(m_From, m_Pos);
			// Prevent infinite bounces
			if(Distance == 0.0f && m_ZeroEnergyBounceInLastTick)
			{
				m_Energy = -1;
			}
			else if(!m_TuneZone)
			{
				m_Energy -= Distance + Tuning()->m_LaserBounceCost;
			}
			else
			{
				m_Energy -= distance(m_From, m_Pos) + GameServer()->TuningList()[m_TuneZone].m_LaserBounceCost;
			}
			m_ZeroEnergyBounceInLastTick = Distance == 0.0f;

			if(Res == TILE_TELEINWEAPON && !GameServer()->Collision()->TeleOuts(z - 1).empty())
			{
				int TeleOut = GameServer()->m_World.m_Core.RandomOr0(GameServer()->Collision()->TeleOuts(z - 1).size());
				m_TelePos = GameServer()->Collision()->TeleOuts(z - 1)[TeleOut];
				m_WasTele = true;
			}
			else
			{
				m_Bounces++;
				m_WasTele = false;
			}

			int BounceNum = Tuning()->m_LaserBounceNum;
			if(m_TuneZone)
				BounceNum = TuningList()[m_TuneZone].m_LaserBounceNum;

			if(m_Bounces > BounceNum)
				m_Energy = -1;

			GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE, m_TeamMask);
		}
	}
	else
	{
		if(!HitCharacterPvp(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		}
	}

	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(m_Owner >= 0 && m_Energy <= 0 && !m_TeleportCancelled && pOwnerChar &&
		pOwnerChar->IsAlive() && pOwnerChar->HasTelegunLaser() && m_Type == WEAPON_LASER)
	{
		vec2 PossiblePos;
		bool Found = false;

		// Check if the laser hits a player.
		bool pDontHitSelf = g_Config.m_SvOldLaser || (m_Bounces == 0 && !m_WasTele);
		vec2 At;
		CCharacter *pHit;
		if(pOwnerChar ? (!pOwnerChar->LaserHitDisabled() && m_Type == WEAPON_LASER) : g_Config.m_SvHit)
			pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pDontHitSelf ? pOwnerChar : nullptr, m_Owner);
		else
			pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pDontHitSelf ? pOwnerChar : nullptr, m_Owner, pOwnerChar);

		if(pHit)
			Found = GetNearestAirPosPlayer(pHit->m_Pos, &PossiblePos);
		else
			Found = GetNearestAirPos(m_Pos, m_From, &PossiblePos);

		if(Found)
		{
			pOwnerChar->m_TeleGunPos = PossiblePos;
			pOwnerChar->m_TeleGunTeleport = true;
			pOwnerChar->m_IsBlueTeleGunTeleport = m_IsBlueTeleport;
		}
	}
	else if(m_Owner >= 0)
	{
		int MapIndex = GameServer()->Collision()->GetPureMapIndex(Coltile);
		int TileFIndex = GameServer()->Collision()->GetFrontTileIndex(MapIndex);
		bool IsSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_TELE_GUN;
		bool IsBlueSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_BLUE_TELE_GUN;
		int IsTeleInWeapon = GameServer()->Collision()->IsTeleportWeapon(MapIndex);

		if(!IsTeleInWeapon)
		{
			if(IsSwitchTeleGun || IsBlueSwitchTeleGun)
			{
				// Delay specifies which weapon the tile should work for.
				// Delay = 0 means all.
				int delay = GameServer()->Collision()->GetSwitchDelay(MapIndex);

				if((delay != 3 && delay != 0) && m_Type == WEAPON_LASER)
				{
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
				}
			}

			m_IsBlueTeleport = TileFIndex == TILE_ALLOW_BLUE_TELE_GUN || IsBlueSwitchTeleGun;

			// Teleport is canceled if the last bounce tile is not a TILE_ALLOW_TELE_GUN.
			// Teleport also works if laser didn't bounce.
			m_TeleportCancelled =
				m_Type == WEAPON_LASER && (TileFIndex != TILE_ALLOW_TELE_GUN && TileFIndex != TILE_ALLOW_BLUE_TELE_GUN && !IsSwitchTeleGun && !IsBlueSwitchTeleGun);
		}
	}

	//m_Owner = -1;
}

void CPvpLaser::Tick()
{
	if((g_Config.m_SvDestroyLasersOnDeath || m_BelongsToPracticeTeam) && m_Owner >= 0)
	{
		CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
		if(!(pOwnerChar && pOwnerChar->IsAlive()))
		{
			Reset();
		}
	}

	float Delay;
	if(m_TuneZone)
		Delay = TuningList()[m_TuneZone].m_LaserBounceDelay;
	else
		Delay = Tuning()->m_LaserBounceDelay;

	if((Server()->Tick() - m_EvalTick) > (Server()->TickSpeed() * Delay / 1000.0f))
		DoBouncePvp();
}
