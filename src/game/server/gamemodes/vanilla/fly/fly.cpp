#include "fly.h"

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/instagib/entities/ddnet_pvp/vanilla_pickup.h>
#include <game/server/player.h>

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
	int OldScore = pVictim->GetPlayer()->m_Score.value_or(0);

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

	// TODO: this hack should be removed by using the config sv_suicide_penalty
	//       and default configs per gametype
	//       see https://github.com/ddnet-insta/ddnet-insta/pull/433
	//       and https://github.com/ddnet-insta/ddnet-insta/issues/308

	// if the player is punished for the selfkill
	// we revert to the original score
	// because in fly selfkills or running into spikes
	// is not supposed to decrement the score
	int NewScore = pVictim->GetPlayer()->m_Score.value_or(0);
	if(NewScore + 1 == OldScore)
	{
		pVictim->GetPlayer()->m_Score = OldScore;
	}

	return ModeSpecial;
}

REGISTER_GAMEMODE(fly, CGameControllerFly(pGameServer));
