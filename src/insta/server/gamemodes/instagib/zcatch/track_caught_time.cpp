#include "zcatch.h"

#include <base/log.h>

#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

#include <insta/server/gamemodes/instagib/base_instagib.h>

#include <optional>

void CGameControllerZcatch::UpdateCatchTicks(class CPlayer *pPlayer, ECatchUpdate Update)
{
	char aBuf[512];
	int Ticks;

	switch(Update)
	{
	case ECatchUpdate::CONNECT:
		pPlayer->m_DeadSinceTick = std::nullopt;
		pPlayer->m_AliveSinceTick = std::nullopt;

		// TODO: it is bad that this logic is duplicated in OnPlayerConnect
		//       not sure which one should be responsible but not both
		if(IsCatchGameRunning() && IsGameRunning())
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
		if(!pPlayer->m_DeadSinceTick.has_value() && !pPlayer->m_AliveSinceTick.has_value())
		{
			str_format(aBuf, sizeof(aBuf), "'%s' stopped playing but no ticks were counted yet", Server()->ClientName(pPlayer->GetCid()));
			if(g_Config.m_SvDebugCatch)
				SendChat(-1, TEAM_ALL, aBuf);
			return;
		}
		dbg_assert(
			!pPlayer->m_DeadSinceTick.has_value() || !pPlayer->m_AliveSinceTick.has_value(),
			"round end and player '%s' has both alive and dead tick counters set",
			Server()->ClientName(pPlayer->GetCid()));

		if(pPlayer->m_DeadSinceTick.has_value())
		{
			Ticks = Server()->Tick() - pPlayer->m_DeadSinceTick.value();
			// TODO: this triggers on round end? Can we keep it in somehow?
			// dbg_assert(pPlayer->m_IsDead == true, "alive player had dead tick set on round end");

			str_format(aBuf, sizeof(aBuf), "'%s' was caught for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			pPlayer->m_Stats.m_TicksDead += Ticks;
		}
		else if(pPlayer->m_AliveSinceTick.has_value())
		{
			Ticks = Server()->Tick() - pPlayer->m_AliveSinceTick.value();
			dbg_assert(pPlayer->m_IsDead == false, "dead player had alive tick set on round end");

			str_format(aBuf, sizeof(aBuf), "'%s' was in game for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			pPlayer->m_Stats.m_TicksAlive += Ticks;
		}

		pPlayer->m_DeadSinceTick = std::nullopt;
		pPlayer->m_AliveSinceTick = std::nullopt;

		if(g_Config.m_SvDebugCatch)
			SendChat(-1, TEAM_ALL, aBuf);

		break;
	case ECatchUpdate::CAUGHT:
		dbg_assert(pPlayer->m_IsDead == false, "dead player with cid %d has been caught by %d? again?", pPlayer->GetCid(), pPlayer->m_KillerId);
		dbg_assert(!pPlayer->m_DeadSinceTick.has_value(), "player has been caught but already has dead ticks set");
		dbg_assert(pPlayer->m_AliveSinceTick.has_value(), "player has been caught but has no alive ticks set");
		Ticks = Server()->Tick() - pPlayer->m_AliveSinceTick.value();
		if(Ticks)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got caught and was alive for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			if(g_Config.m_SvDebugCatch)
				SendChat(-1, TEAM_ALL, aBuf);
			pPlayer->m_Stats.m_TicksAlive += Ticks;
		}
		else if(g_Config.m_SvDebugCatch)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got caught but was alive for 0 ticks so we ignore it", Server()->ClientName(pPlayer->GetCid()));
			SendChat(-1, TEAM_ALL, aBuf);
		}

		pPlayer->m_DeadSinceTick = Server()->Tick();
		pPlayer->m_AliveSinceTick = std::nullopt;
		break;
	case ECatchUpdate::RELEASE:
		dbg_assert(!pPlayer->m_AliveSinceTick.has_value(), "player has been released but already has alive ticks set");
		dbg_assert(pPlayer->m_DeadSinceTick.has_value(), "player has been released but has no dead ticks set");
		dbg_assert(pPlayer->m_IsDead == true, "alive player has been released cid=%d alive_since_s=%d", pPlayer->GetCid(), (Server()->Tick() - pPlayer->m_AliveSinceTick.value()) / Server()->TickSpeed());

		Ticks = Server()->Tick() - pPlayer->m_DeadSinceTick.value();
		if(Ticks)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got released and was dead for %d ticks", Server()->ClientName(pPlayer->GetCid()), Ticks);
			if(g_Config.m_SvDebugCatch)
				SendChat(-1, TEAM_ALL, aBuf);
			pPlayer->m_Stats.m_TicksDead += Ticks;
		}
		else if(g_Config.m_SvDebugCatch)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' got released but was dead for 0 ticks so we ignore it", Server()->ClientName(pPlayer->GetCid()));
			SendChat(-1, TEAM_ALL, aBuf);
		}

		pPlayer->m_DeadSinceTick = std::nullopt;
		pPlayer->m_AliveSinceTick = Server()->Tick();
		break;
	}

	if(!IsCatchGameRunning())
		dbg_assert(!pPlayer->m_DeadSinceTick.has_value() && !pPlayer->m_AliveSinceTick.has_value(), "no catch game is running but a player is tracking caught/alive time");
}
