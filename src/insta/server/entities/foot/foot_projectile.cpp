#include "foot_projectile.h"

#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

CFootProjectile::CFootProjectile(
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
	if((Dir.x < 0 ? -Dir.x : Dir.x) > (Dir.y < 0 ? -Dir.y : Dir.y))
		m_FootPickupDistance = std::abs(Dir.x * static_cast<float>(Server()->TickSpeed()) * GameServer()->GlobalTuning()->m_GrenadeSpeed / 4000.0);
	else
		m_FootPickupDistance = std::abs(Dir.y * static_cast<float>(Server()->TickSpeed()) * GameServer()->GlobalTuning()->m_GrenadeSpeed / 4000.0);

	if((m_Owner < 0) || (m_Owner >= MAX_CLIENTS) || !GameServer()->m_apPlayers[m_Owner])
	{
		m_Owner = -1;
		m_Team = -1;
	}
	else
		m_Team = GameServer()->GetDDRaceTeam(m_Owner);
}

void CFootProjectile::DestroyBall(const vec2 &Pos, int Team = -1, CClientMask TeamMask = CClientMask().set())
{
	GameServer()->CreateSound(Pos, m_SoundImpact, TeamMask);
	if(g_Config.m_SvBallExplode == 1)
		GameServer()->CreateExplosion(m_Pos, -1, WEAPON_GRENADE, true, m_DDRaceTeam, TeamMask, m_AffectedCharacters);
	GameServer()->m_pController->BallReset(Team, 4);
	m_MarkedForDestroy = true;
}

CCharacter *CFootProjectile::GetPlayerPickup(const vec2 &PrevPos, const vec2 &CurPos, CCharacter *pOwnerChar)
{
	CCharacter *pChr = static_cast<CCharacter *>(GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER));
	for(; pChr; pChr = static_cast<CCharacter *>(pChr->TypeNext()))
	{
		if(m_FootPickupDistance != 0 && pOwnerChar && pChr == pOwnerChar)
			continue;

		const int Team = pChr->Team();
		if(Team < 0 || Team >= NUM_DDRACE_TEAMS || pChr->Team() != m_Team)
			continue;

		vec2 IntersectPos;
		if(!closest_point_on_line(PrevPos, CurPos, pChr->m_Pos, IntersectPos))
			continue;

		float DistToLine = distance(pChr->m_Pos, IntersectPos);
		if(DistToLine > pChr->GetProximityRadius() + GetProximityRadius() || distance(PrevPos, IntersectPos) > distance(PrevPos, CurPos))
			continue;

		pChr->PlayerGetBall();
		pChr->m_BallLastTouchedBy = m_Owner;
		pChr->m_BallLastTouchedTeam = (pOwnerChar ? pOwnerChar : pChr)->GetPlayer()->GetTeam();
		return pChr;
	}

	return nullptr;
}

void CFootProjectile::Tick()
{
	const float TickSpeed = static_cast<float>(Server()->TickSpeed());

	const float PrevTick = (Server()->Tick() - m_StartTick - 1) / TickSpeed;
	const float CurTick = (Server()->Tick() - m_StartTick) / TickSpeed;
	const float NextTick = (Server()->Tick() - m_StartTick + 1) / TickSpeed;

	const float TickDelta = NextTick - CurTick;

	vec2 PrevPos = GetPos(PrevTick);
	vec2 CurPos = GetPos(CurTick);

	vec2 CollisionPos = vec2(0.0f, 0.0f);
	vec2 FreePos = vec2(0.0f, 0.0f);

	CCharacter *pOwnerChar = m_Owner >= 0 ? GameServer()->GetPlayerChar(m_Owner) : nullptr;
	CClientMask TeamMask = pOwnerChar && pOwnerChar->IsAlive() ? pOwnerChar->TeamMask() : CClientMask().set();
	const int Team = pOwnerChar ? pOwnerChar->Team() : -1;

	if(m_LifeSpan > -1)
		m_LifeSpan--;

	float TimeToCollision = -1.0f;
	constexpr int Steps = 30;

	for(int i = 0; i <= Steps; ++i)
	{
		const float SearchTick = std::lerp(CurTick, NextTick, static_cast<float>(i) / Steps);
		if(const vec2 t = GetPos(SearchTick); Collision()->IsSolid((int)t.x, (int)t.y))
			break;
		TimeToCollision = SearchTick;
	}

	if(TimeToCollision == -1.0f)
	{
		m_FootPickupDistance = 0;

		constexpr float StepSize = 0.02f;
		const int NumSteps = static_cast<int>(std::ceil(1.0f / StepSize));

		for(int i = 0; i < NumSteps; ++i)
		{
			float t = CurTick - i * StepSize;
			if(t < CurTick - 1.0f)
				break;

			vec2 Pos = GetPos(t);
			if(!Collision()->IsSolid((int)Pos.x, (int)Pos.y))
			{
				TimeToCollision = t;
				CollisionPos = GetPos(t + StepSize);
				FreePos = Pos;
				break;
			}
		}
	}
	else
	{
		TimeToCollision += CurTick;
		CollisionPos = GetPos(TimeToCollision + CurTick + TickDelta / Steps);
		FreePos = GetPos(TimeToCollision + CurTick);
	}

	if(const int Tile = GameServer()->Collision()->GetTileIndex(GameServer()->Collision()->GetMapIndex(CurPos)); Tile != TILE_AIR)
		m_LastTouchedTile = Tile;

	if(g_Config.m_SvKillTileDestroysBall)
	{
		if(GameServer()->Collision()->GetCollisionAt(CurPos.x, CurPos.y) == TILE_DEATH || GameServer()->Collision()->GetFrontCollisionAt(CurPos.x, CurPos.y) == TILE_DEATH || GameLayerClipped(CurPos))
		{
			GameServer()->CreateSound(CurPos, m_SoundImpact, TeamMask);
			GameServer()->m_pController->BallReset(Team, 4);
			m_MarkedForDestroy = true;
			return;
		}
	}

	if(TimeToCollision < NextTick - TickDelta / Steps)
	{
		bool CollidedX = false;
		bool CollidedY = false;
		if(GameServer()->Collision()->IsSolid((int)CollisionPos.x, (int)FreePos.y))
			CollidedX = true;
		if(GameServer()->Collision()->IsSolid((int)FreePos.x, (int)CollisionPos.y))
			CollidedY = true;

		const float BounceFactor = g_Config.m_SvBallBounceFriction + 100.0f;
		const float GrenadeEffect = 2.0f * GameServer()->GlobalTuning()->m_GrenadeCurvature / 10000.0f * GameServer()->GlobalTuning()->m_GrenadeSpeed * (Server()->Tick() - m_StartTick) / TickSpeed;

		if(CollidedX)
		{
			m_Direction.x = -m_Direction.x / BounceFactor * 100.0f;
			if(++m_CollisionByX >= 50)
			{
				DestroyBall(CurPos, Team, TeamMask);
				return;
			}
		}
		else
		{
			m_Direction.x = m_Direction.x / BounceFactor * 100.0f;
			m_CollisionByX = 0;
		}

		if(CollidedY)
		{
			m_Direction.y = -(m_Direction.y + GrenadeEffect) / BounceFactor * 100.0f;
			if(++m_CollisionByY >= 50)
			{
				DestroyBall(CurPos, Team, TeamMask);
				return;
			}
		}
		else
		{
			m_Direction.y = (m_Direction.y + GrenadeEffect) / BounceFactor * 100.0f;
			m_CollisionByY = 0;
		}

		m_Pos = FreePos;
		m_StartTick = Server()->Tick();
		m_FootPickupDistance = 0;
	}

	if(GetPlayerPickup(PrevPos, CurPos, pOwnerChar))
		m_MarkedForDestroy = true;

	if(m_FootPickupDistance > 0)
		m_FootPickupDistance--;
}

void CFootProjectile::Snap(int SnappingClient)
{
	const float Ct = (Server()->Tick() - m_StartTick) / static_cast<float>(Server()->TickSpeed());
	if(NetworkClipped(SnappingClient, GetPos(Ct)))
		return;

	const int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	if(SnappingClientVersion < VERSION_DDNET_ENTITY_NETOBJS)
	{
		CCharacter *pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
		const int Tick = (Server()->Tick() % Server()->TickSpeed()) % ((m_Explosive) ? 6 : 20);
		if(pSnapChar && pSnapChar->IsAlive() && m_Layer == LAYER_SWITCH && m_Number > 0 && !Switchers()[m_Number].m_aStatus[pSnapChar->Team()] && !Tick)
			return;
	}

	CCharacter *pOwnerChar = m_Owner >= 0 ? GameServer()->GetPlayerChar(m_Owner) : nullptr;
	CClientMask TeamMask = pOwnerChar && pOwnerChar->IsAlive() ? pOwnerChar->TeamMask() : CClientMask().set();
	if(SnappingClient != SERVER_DEMO_CLIENT && m_Owner != -1 && !TeamMask.test(SnappingClient))
		return;

	CNetObj_Projectile *pProj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, GetId(), sizeof(CNetObj_Projectile)));
	if(pProj)
		FillInfo(pProj);
}
