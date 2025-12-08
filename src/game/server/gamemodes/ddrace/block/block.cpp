#include "block.h"

#include <base/system.h>

#include <engine/server.h>
#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamemodes/base_pvp/base_pvp.h>
#include <game/server/player.h>

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

int CGameControllerBlock::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// this is a edge case
	// probably caused by admin abuse or something like that
	// this can only happen if a player was killed by damage
	// which should not happen in block
	if(pKiller && pKiller != pVictim->GetPlayer())
	{
		return CGameControllerBasePvp::OnCharacterDeath(pVictim, pKiller, Weapon);
	}
	std::optional<CLastToucher> &LastToucher = pVictim->GetPlayer()->m_LastToucher;

	// died alone without any killer
	if(!LastToucher.has_value())
		return 0; // do not count the kill

	int LastToucherId = LastToucher.value().m_ClientId;
	if(LastToucherId >= 0 && LastToucherId < MAX_CLIENTS)
		pKiller = GameServer()->m_apPlayers[LastToucherId];

	if(pKiller && pKiller != pVictim->GetPlayer() && pVictim->m_FreezeTime)
	{
		OnKill(pVictim->GetPlayer(), pKiller, Weapon);
		pKiller->IncrementScore();

		// TODO: the kill message will also be sent in CCharacter::Die which is a bit annoying
		int KillMsgWeapon = LastToucher.value().m_Weapon;
		if(KillMsgWeapon == WEAPON_HOOK)
			KillMsgWeapon = WEAPON_NINJA;

		// kill message
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = pKiller->GetCid();
		Msg.m_Victim = pVictim->GetPlayer()->GetCid();
		Msg.m_Weapon = KillMsgWeapon;
		Msg.m_ModeSpecial = 0;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);

		// count the kill
		return CGameControllerBasePvp::OnCharacterDeath(pVictim, pKiller, Weapon);
	}

	// do not count the kill
	return 0;
}

REGISTER_GAMEMODE(block, CGameControllerBlock(pGameServer));
