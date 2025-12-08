#include "foot_pickup.h"

#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/ball/base_foot.h>
#include <game/server/player.h>

CFootPickup::CFootPickup(CGameWorld *pGameWorld, int Layer, int Number) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP, vec2(0.0f, 0.0f), PICKUP_PHYS_SIZE)
{
	m_Type = POWERUP_WEAPON;
	m_Subtype = WEAPON_GRENADE;

	m_Layer = Layer;
	m_Number = Number;

	for(int &SpawnTick : m_aSpawnTickTeam)
		SpawnTick = -1;

	GameWorld()->InsertEntity(this);
}

void CFootPickup::Reset()
{
	m_MarkedForDestroy = true;
}

void CFootPickup::Tick()
{
	// TODO: ideally this cast would be removed and foot pickups work properly in all modes
	CGameControllerBaseFoot *pController = dynamic_cast<CGameControllerBaseFoot *>(GameServer()->m_pController);
	dbg_assert(pController != nullptr, "foot pickup can only be used by foot controllers");

	for(CEntity *pEnt = GameWorld()->FindFirst(CGameWorld::ENTTYPE_CHARACTER); pEnt; pEnt = pEnt->TypeNext())
	{
		CCharacter *pChr = dynamic_cast<CCharacter *>(pEnt);
		if(!pChr)
			continue;

		const int Team = pChr->Team();
		if(Team < 0 || Team >= NUM_DDRACE_TEAMS)
			continue;

		if(pController->m_aBallTickSpawning[Team] && m_aSpawnTickTeam[Team] != -1 && pController->m_aBallTickSpawning[Team] <= Server()->Tick())
		{
			// respawn
			m_aSpawnTickTeam[Team] = -1;
			if(m_Type == POWERUP_WEAPON)
				GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SPAWN, pChr->TeamMask());
		}
		else if((!pController->m_aBallTickSpawning[Team] && m_aSpawnTickTeam[Team] == 0) || pController->m_aBallTickSpawning[Team] > Server()->Tick())
			continue;

		if(distance(m_Pos, pChr->m_Pos) > (GetProximityRadius() + CPickup::ms_CollisionExtraSize + pChr->GetProximityRadius()))
			continue;

		if(!pChr->GetWeaponGot(WEAPON_GRENADE) || pChr->GetWeaponAmmo(WEAPON_GRENADE) != -1)
		{
			//give ball
			m_aSpawnTickTeam[Team] = 0;
			GameServer()->CreateSound(m_Pos, SOUND_PICKUP_GRENADE, pChr->TeamMask());

			pController->m_aBallTickSpawning[Team] = 0;
			pChr->PlayerGetBall();
		}
	}
}

void CFootPickup::TickPaused()
{
	for(int &SpawnTick : m_aSpawnTickTeam)
		if(SpawnTick != -1)
			++SpawnTick;
}

void CFootPickup::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CCharacter *pChar = GameServer()->GetPlayerChar(SnappingClient);
	if(!pChar)
		return;

	const int Team = pChar->Team();
	if(Team < 0 || Team >= NUM_DDRACE_TEAMS)
		return;

	if(m_aSpawnTickTeam[Team] != -1)
		return;

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	bool Sixup = Server()->IsSixup(SnappingClient);

	if(SnappingClientVersion < VERSION_DDNET_ENTITY_NETOBJS)
	{
		if(SnappingClient != SERVER_DEMO_CLIENT && (GameServer()->m_apPlayers[SnappingClient]->GetTeam() == TEAM_SPECTATORS || GameServer()->m_apPlayers[SnappingClient]->IsPaused()) && GameServer()->m_apPlayers[SnappingClient]->SpectatorId() != SPEC_FREEVIEW)
			pChar = GameServer()->GetPlayerChar(GameServer()->m_apPlayers[SnappingClient]->SpectatorId());

		int Tick = (Server()->Tick() % Server()->TickSpeed()) % 11;
		if(pChar && pChar->IsAlive() && m_Layer == LAYER_SWITCH && m_Number > 0 && !Switchers()[m_Number].m_aStatus[Team] && !Tick)
			return;
	}

	GameServer()->SnapPickup(CSnapContext(SnappingClientVersion, Sixup, SnappingClient), GetId(), m_Pos, m_Type, m_Subtype, m_Number, PICKUPFLAG_NO_PREDICT);
}

void CFootPickup::Move()
{
	if(Server()->Tick() % static_cast<int>(Server()->TickSpeed() * 0.15f) == 0)
	{
		Collision()->MoverSpeed(m_Pos.x, m_Pos.y, &m_Core);
		m_Pos += m_Core;
	}
}
