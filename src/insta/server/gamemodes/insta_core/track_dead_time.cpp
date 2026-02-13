#include <engine/shared/config.h>
#include <insta/server/gamemodes/insta_core/insta_core.h>

void CGameControllerInstaCore::UpdateDeadTicks(CPlayer *pPlayer, EDeadUpdate Update)
{
	// zCatch for now has its own tracker
	// but ideally they should be merged
	if(IsZcatchGameType())
		return;

	char aBuf[512];
	int Ticks;

	switch (Update) {
		case EDeadUpdate::CONNECT:
			pPlayer->m_DeadSinceTick = std::nullopt;
			pPlayer->m_AliveSinceTick = std::nullopt;
		break;
		case EDeadUpdate::DEATH:
		break;
		case EDeadUpdate::FREEZE:
		break;
		case EDeadUpdate::UNFREEZE:
		break;
		case EDeadUpdate::SPAWN:
		break;
		case EDeadUpdate::SPECTATE:
		case EDeadUpdate::DISCONNECT:
		case EDeadUpdate::ROUND_END:
		// there are cases where we never track
		// for example tournament mode servers
		// warmup phase
		// or any other non ready game state
		if(!pPlayer->m_DeadSinceTick.has_value() && !pPlayer->m_AliveSinceTick.has_value())
		{
			str_format(aBuf, sizeof(aBuf), "'%s' stopped playing but no ticks were counted yet", Server()->ClientName(pPlayer->GetCid()));
			if(g_Config.m_SvDebugDeadTracker)
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
	}
}
