#include <base/log.h>
#include <base/system.h>
#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/generated/protocol7.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/flag.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/gamemodes/instagib/base_instagib.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include "zcatch.h"

void CGameControllerZcatch::UpdateCatchTicks(class CPlayer *pPlayer, ECatchUpdate Update)
{
	char aBuf[512];
	int Ticks;

	switch(Update)
	{
	case ECatchUpdate::CONNECT:
		pPlayer->m_DeadSinceTick = 0;
		pPlayer->m_AliveSinceTick = 0;

		if(IsCatchGameRunning())
		{
			pPlayer->m_AliveSinceTick = Server()->Tick();
			str_format(aBuf, sizeof(aBuf), "'%s' started playing (counting alive ticks)", Server()->ClientName(pPlayer->GetCid()));
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "'%s' started playing (not counting ticks during release game)", Server()->ClientName(pPlayer->GetCid()));
		}

		if(g_Config.m_SvDebugCatch)
			SendChat(-1, TEAM_ALL, aBuf);

		break;
	case ECatchUpdate::DISCONNECT:
	case ECatchUpdate::ROUND_END:
	case ECatchUpdate::SPECTATE:

		// if it was a release game we dont track
		if(pPlayer->m_DeadSinceTick == 0 && pPlayer->m_AliveSinceTick == 0)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' stopped playing but no ticks were counted yet", Server()->ClientName(pPlayer->GetCid()));
			if(g_Config.m_SvDebugCatch)
				SendChat(-1, TEAM_ALL, aBuf);
			return;
		}
		str_format(aBuf, sizeof(aBuf), "round end and player '%s' has both alive and dead tick counters set", Server()->ClientName(pPlayer->GetCid()));
		dbg_assert(pPlayer->m_DeadSinceTick == 0 || pPlayer->m_AliveSinceTick == 0, aBuf);

		if(pPlayer->m_DeadSinceTick)
		{
			Ticks = Server()->Tick() - pPlayer->m_DeadSinceTick;
			// TODO: this triggers on round end? Can we keep it in somehow?
			// dbg_assert(pPlayer->m_IsDead == true, "alive player had dead tick set on round end");

			str_format(aBuf, sizeof(aBuf), "'%s' was caught for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			pPlayer->m_Stats.m_TicksCaught += Ticks;
		}
		else if(pPlayer->m_AliveSinceTick)
		{
			Ticks = Server()->Tick() - pPlayer->m_AliveSinceTick;
			dbg_assert(pPlayer->m_IsDead == false, "dead player had alive tick set on round end");

			str_format(aBuf, sizeof(aBuf), "'%s' was in game for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			pPlayer->m_Stats.m_TicksInGame += Ticks;
		}

		pPlayer->m_DeadSinceTick = 0;
		pPlayer->m_AliveSinceTick = 0;

		if(g_Config.m_SvDebugCatch)
			SendChat(-1, TEAM_ALL, aBuf);

		break;
	case ECatchUpdate::CAUGHT:
		dbg_assert(pPlayer->m_IsDead == false, "dead player has been caught? again?");
		dbg_assert(pPlayer->m_DeadSinceTick == 0, "player has been caught but already has dead ticks set");
		Ticks = Server()->Tick() - pPlayer->m_AliveSinceTick;
		if(Ticks)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got caught and was alive for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			if(g_Config.m_SvDebugCatch)
				SendChat(-1, TEAM_ALL, aBuf);
			pPlayer->m_Stats.m_TicksInGame += Ticks;
		}
		else if(g_Config.m_SvDebugCatch)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got caught but was alive for 0 ticks so we ignore it", Server()->ClientName(pPlayer->GetCid()));
			SendChat(-1, TEAM_ALL, aBuf);
		}

		pPlayer->m_DeadSinceTick = Server()->Tick();
		pPlayer->m_AliveSinceTick = 0;
		break;
	case ECatchUpdate::RELEASE:
		dbg_assert(pPlayer->m_IsDead == true, "alive player has been released cid=%d alive_since_s=%d", pPlayer->GetCid(), (Server()->Tick() - pPlayer->m_AliveSinceTick) / Server()->TickSpeed());
		dbg_assert(pPlayer->m_AliveSinceTick == 0, "player has been released but already has alive ticks set");

		Ticks = Server()->Tick() - pPlayer->m_DeadSinceTick;
		if(Ticks)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got released and was dead for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			if(g_Config.m_SvDebugCatch)
				SendChat(-1, TEAM_ALL, aBuf);
			pPlayer->m_Stats.m_TicksCaught += Ticks;
		}
		else if(g_Config.m_SvDebugCatch)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got released but was dead for 0 ticks so we ignore it", Server()->ClientName(pPlayer->GetCid()));
			SendChat(-1, TEAM_ALL, aBuf);
		}

		pPlayer->m_DeadSinceTick = 0;
		pPlayer->m_AliveSinceTick = Server()->Tick();
		break;
	}

	if(!IsCatchGameRunning())
		dbg_assert(pPlayer->m_DeadSinceTick == 0 && pPlayer->m_AliveSinceTick == 0, "no catch game is running but a player is tracking caught/alive time");
}
