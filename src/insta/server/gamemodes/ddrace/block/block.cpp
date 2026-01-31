#include "block.h"

#include <base/system.h>

#include <engine/server.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/gamemodes/base_pvp/base_pvp.h>

#include <optional>

CGameControllerBlock::CGameControllerBlock(class CGameContext *pGameServer) :
	CGameControllerBasePvp(pGameServer)
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
	CGameControllerBasePvp::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
	pChr->GiveWeapon(WEAPON_GUN, false, 10);
}

void CGameControllerBlock::Tick()
{
	for(CPlayer *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->ResetLastToucherAfterSeconds(3);
	}

	// keep last to
	// make sure the pvp ticks set the hooking toucher
	// even if we did reset it this tick
	CGameControllerBasePvp::Tick();
}

bool CGameControllerBlock::SkipDamage(int Dmg, int From, int Weapon, const CCharacter *pCharacter, bool &ApplyForce)
{
	ApplyForce = true;

	// there is never damage in block
	// it is ddrace like
	return true;
}

void CGameControllerBlock::OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg)
{
	CPlayer *pKiller = GetPlayerOrNullptr(Killer);
	// this is a edge case
	// probably caused by admin abuse or something like that
	// this can only happen if a player was killed by damage
	// which should not happen in block
	if(pKiller && pKiller != pVictim->GetPlayer())
	{
		CGameControllerBasePvp::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
		return;
	}
	std::optional<CLastToucher> &LastToucher = pVictim->GetPlayer()->m_LastToucher;

	// died alone without any killer
	if(!LastToucher.has_value())
	{
		// do not count the kill
		CGameControllerBasePvp::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
		return;
	}

	int LastToucherId = LastToucher.value().m_ClientId;
	if(LastToucherId >= 0 && LastToucherId < MAX_CLIENTS)
		pKiller = GameServer()->m_apPlayers[LastToucherId];

	if(pKiller && pKiller != pVictim->GetPlayer() && pVictim->m_FreezeTime)
	{
		int KillMsgWeapon = LastToucher.value().m_Weapon;
		if(KillMsgWeapon == WEAPON_HOOK)
			KillMsgWeapon = WEAPON_NINJA;

		// count the kill
		CGameControllerBasePvp::OnCharacterDeathImpl(pVictim, pKiller->GetCid(), KillMsgWeapon, SendKillMsg);
		return;
	}

	// do not count the kill
	CGameControllerBasePvp::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
}

REGISTER_GAMEMODE(block, CGameControllerBlock(pGameServer));
