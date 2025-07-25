/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "character.h"
#include <game/mapitems.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>

#include <game/server/player.h>

#include "flag.h"
CFlag::CFlag(CGameWorld *pGameWorld, int Team) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG)
{
	m_IsGrounded = true;
	m_Team = Team;
	m_pCarrier = NULL;
	m_GrabTick = 0;

	Reset();
}

void CFlag::Reset()
{
	m_IsGrounded = true;
	m_pLastCarrier = nullptr;
	m_pCarrier = nullptr;
	m_AtStand = true;
	m_Pos = m_StandPos;
	m_Vel = vec2(0, 0);
	m_GrabTick = 0;
}

void CFlag::Grab(CCharacter *pChar)
{
	m_pCarrier = pChar;

	GameServer()->m_pController->OnFlagGrab(this);

	if(m_AtStand)
		m_GrabTick = Server()->Tick();
	m_AtStand = false;

	for(int c = 0; c < MAX_CLIENTS; c++)
	{
		CPlayer *pPlayer = GameServer()->m_apPlayers[c];
		if(!pPlayer)
			continue;

		if(pPlayer->GetTeam() == TEAM_SPECTATORS && pPlayer->SpectatorId() != SPEC_FREEVIEW && GameServer()->m_apPlayers[pPlayer->SpectatorId()] && GameServer()->m_apPlayers[pPlayer->SpectatorId()]->GetTeam() == m_Team)
			GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, c);
		else if(pPlayer->GetTeam() == m_Team)
			GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, c);
		else
			GameServer()->CreateSoundGlobal(SOUND_CTF_GRAB_PL, c);
	}
}

void CFlag::Drop(vec2 Direction)
{
	m_pLastCarrier = m_pCarrier;
	m_pCarrier = 0;
	m_Vel = Direction;
	m_DropTick = Server()->Tick();
}

void CFlag::TickDeferred()
{
	if(m_pCarrier)
	{
		// update flag position
		m_Pos = m_pCarrier->GetPos();
	}
	else
	{
		// flag hits death-tile or left the game layer, reset it
		if((GameServer()->Collision()->GetCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH) ||
			(GameServer()->Collision()->GetFrontCollisionAt(m_Pos.x, m_Pos.y) == TILE_DEATH) ||
			GameLayerClipped(m_Pos))
		{
			Reset();
			GameServer()->m_pController->OnFlagReturn(this);
		}

		if(!m_AtStand)
		{
			if(Server()->Tick() > m_DropTick + Server()->TickSpeed() * 30)
			{
				Reset();
				GameServer()->m_pController->OnFlagReturn(this);
			}
			else
			{
				// Friction
				m_IsGrounded = false;
				if(GameServer()->Collision()->CheckPoint(m_Pos.x + CFlag::ms_PhysSize / 2, m_Pos.y + CFlag::ms_PhysSize / 2 + 5))
					m_IsGrounded = true;
				if(GameServer()->Collision()->CheckPoint(m_Pos.x - CFlag::ms_PhysSize / 2, m_Pos.y + CFlag::ms_PhysSize / 2 + 5))
					m_IsGrounded = true;

				if(m_IsGrounded)
				{
					m_Vel.x *= 0.75f;
				}
				else
				{
					m_Vel.x *= 0.98f;
				}

				// Gravity
				m_Vel.y += GameWorld()->m_Core.m_aTuning[0].m_Gravity;
				GameServer()->Collision()->MoveBox(&m_Pos, &m_Vel, vec2(ms_PhysSize, ms_PhysSize), vec2(0.5, 0.5));
			}
		}
	}
}

void CFlag::TickPaused()
{
	++m_DropTick;
	if(m_GrabTick)
		++m_GrabTick;
}

void CFlag::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	// this should not be here ._.
	// it is also done in TickDeferred and vanilla does not
	// keep this in here
	// but thats the only fix i found for
	// https://github.com/ddnet-insta/ddnet-insta/issues/6
	// in 0.7 the flag laggs behind otherwise unless cl_predict_players is set to 1
	// which is not needed on vanilla servers
	if(m_pCarrier)
		m_Pos = m_pCarrier->GetPos();

	if(Server()->IsSixup(SnappingClient))
	{
		protocol7::CNetObj_Flag *pFlag = Server()->SnapNewItem<protocol7::CNetObj_Flag>(m_Team);
		if(!pFlag)
			return;
		pFlag->m_X = round_to_int(m_Pos.x);
		pFlag->m_Y = round_to_int(m_Pos.y);
		pFlag->m_Team = m_Team;
	}
	else
	{
		CNetObj_Flag *pFlag = Server()->SnapNewItem<CNetObj_Flag>(m_Team);
		if(!pFlag)
			return;
		pFlag->m_X = round_to_int(m_Pos.x);
		pFlag->m_Y = round_to_int(m_Pos.y);
		pFlag->m_Team = m_Team;
	}
}
