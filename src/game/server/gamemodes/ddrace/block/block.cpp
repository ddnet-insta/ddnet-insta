#include <base/system.h>
#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/base_pvp/base_pvp.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include "block.h"

CGameControllerBlock::CGameControllerBlock(class CGameContext *pGameServer) :
	CGameControllerPvp(pGameServer)
{
	m_pGameType = "block";
	m_GameFlags = 0;
	m_DefaultWeapon = WEAPON_GUN;

	m_pStatsTable = "block";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerBlock::~CGameControllerBlock() = default;

void CGameControllerBlock::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerPvp::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

void CGameControllerBlock::Tick()
{
	// TODO: this code is duplicated with fly
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_TicksSinceLastTouch++;
		int SecsSinceTouch = pPlayer->m_TicksSinceLastTouch / Server()->TickSpeed();
		if(SecsSinceTouch > 3)
			pPlayer->UpdateLastToucher(-1);
	}

	// keep last to
	// make sure the pvp ticks set the hooking toucher
	// even if we did reset it this tick
	CGameControllerPvp::Tick();
}

bool CGameControllerBlock::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	// there is never damage in block
	// it is ddrace like
	return true;
}

int CGameControllerBlock::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// this is a edge case
	// probably caused by admin abuse or something like that
	// this can only happen if a player was killed by damage
	// which should not happen in block
	if(pKiller && pKiller != pVictim->GetPlayer())
	{
		return CGameControllerPvp::OnCharacterDeath(pVictim, pKiller, Weapon);
	}

	int LastToucherId = pVictim->GetPlayer()->m_LastToucherId;
	if(LastToucherId >= 0 && LastToucherId < MAX_CLIENTS)
		pKiller = GameServer()->m_apPlayers[LastToucherId];

	if(pKiller && pKiller != pVictim->GetPlayer() && pVictim->m_FreezeTime)
	{
		OnKill(pVictim->GetPlayer(), pKiller, Weapon);

		// TODO: the kill message will also be sent in CCharacter::Die which is a bit annoying

		// kill message
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = pKiller->GetCid();
		Msg.m_Victim = pVictim->GetPlayer()->GetCid();
		Msg.m_Weapon = WEAPON_NINJA; // TODO: track last touch weapon
		Msg.m_ModeSpecial = 0;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

		// count the kill
		return CGameControllerPvp::OnCharacterDeath(pVictim, pKiller, Weapon);
	}

	// do not count the kill
	return 0;
}
