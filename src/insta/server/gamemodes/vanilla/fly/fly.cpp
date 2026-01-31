#include "fly.h"

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/entities/ddnet_pvp/vanilla_pickup.h>

CGameControllerFly::CGameControllerFly(class CGameContext *pGameServer) :
	CGameControllerCTF(pGameServer)
{
	m_pGameType = "fly";
	m_pStatsTable = "fly";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerFly::~CGameControllerFly() = default;

void CGameControllerFly::Tick()
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
	CGameControllerCTF::Tick();
}

int CGameControllerFly::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// spike kills
	const int LastToucherId = pVictim->GetPlayer()->m_LastToucher.has_value() ? pVictim->GetPlayer()->m_LastToucher.value().m_ClientId : -1;
	if(LastToucherId >= 0 && LastToucherId < MAX_CLIENTS)
		pKiller = GameServer()->m_apPlayers[LastToucherId];

	if(pKiller && pKiller != pVictim->GetPlayer() && Weapon == WEAPON_WORLD)
	{
		OnKill(pVictim->GetPlayer(), pKiller, Weapon);

		pKiller->IncrementScore();

		// TODO: the kill message will also be sent in CCharacter::Die which is a bit annoying

		// kill message
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = pKiller->GetCid();
		Msg.m_Victim = pVictim->GetPlayer()->GetCid();
		Msg.m_Weapon = WEAPON_WORLD; // TODO: track last touch weapon
		Msg.m_ModeSpecial = 0;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}
	int ModeSpecial = CGameControllerCTF::OnCharacterDeath(pVictim, pKiller, Weapon);
	return ModeSpecial;
}

REGISTER_GAMEMODE(fly, CGameControllerFly(pGameServer));
