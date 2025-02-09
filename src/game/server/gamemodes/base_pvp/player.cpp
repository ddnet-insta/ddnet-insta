#include <base/log.h>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/entity.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/sql_accounts.h>
#include <game/server/instagib/sql_stats.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

void CPlayer::ResetStats()
{
	m_Kills = 0;
	m_Deaths = 0;
	m_Stats.Reset();
}

void CPlayer::WarmupAlert()
{
	// 0.7 has client side infinite warmup support
	// so we do only need the broadcast for 0.6 players
	if(Server()->IsSixup(GetCid()))
		return;

	m_SentWarmupAlerts++;
	if(m_SentWarmupAlerts < 3)
	{
		GameServer()->SendBroadcast("This is a warmup game. Call a restart vote to start.", GetCid());
	}
}

const char *CPlayer::GetTeamStr() const
{
	if(GetTeam() == TEAM_SPECTATORS)
		return "spectator";

	if(GameServer()->m_pController && !GameServer()->m_pController->IsTeamPlay())
		return "game";

	if(GetTeam() == TEAM_RED)
		return "red";
	return "blue";
}

void CPlayer::AddScore(int Score)
{
	if(GameServer()->m_pController && GameServer()->m_pController->IsWarmup())
	{
		WarmupAlert();
		return;
	}

	// never count score or win rounds in ddrace teams
	if(GameServer()->GetDDRaceTeam(GetCid()))
		return;

	// never decrement the tracked score
	// so fakers can not remove points from others
	if(Score > 0 && GameServer()->m_pController && GameServer()->m_pController->IsStatTrack())
		m_Stats.m_Points += Score;

	m_Score = m_Score.value_or(0) + Score;
}

void CPlayer::AddKills(int Amount)
{
	if(GameServer()->m_pController->IsStatTrack())
		m_Stats.m_Kills += Amount;

	m_Kills += Amount;
}

void CPlayer::AddDeaths(int Amount)
{
	if(GameServer()->m_pController->IsStatTrack())
		m_Stats.m_Deaths += Amount;

	m_Deaths += Amount;
}

void CPlayer::InstagibTick()
{
	// 6 seconds of doing nothing should never happen during
	// a competitive game
	// even tactical waiting (also known as "camping")
	// should not take that long without once moving the mouse
	m_IsCompetitiveAfk = m_LastPlaytime < time_get() - time_freq() * 6;

	if(m_StatsQueryResult != nullptr && m_StatsQueryResult->m_Completed)
	{
		ProcessStatsResult(*m_StatsQueryResult);
		m_StatsQueryResult = nullptr;
	}
	if(m_FastcapQueryResult != nullptr && m_FastcapQueryResult->m_Completed)
	{
		ProcessStatsResult(*m_FastcapQueryResult);
		m_FastcapQueryResult = nullptr;
	}
	if(m_AccountQueryResult != nullptr && m_AccountQueryResult->m_Completed)
	{
		ProcessAccountResult(*m_AccountQueryResult);
		m_AccountQueryResult = nullptr;
	}
	if(m_AccountLogoutQueryResult != nullptr && m_AccountLogoutQueryResult->m_Completed)
	{
		if(!m_AccountLogoutQueryResult->m_Success)
		{
			GameServer()->SendChatTarget(GetCid(), m_AccountLogoutQueryResult->m_aMessage);
		}
		else if(GameServer()->m_pController)
		{
			GameServer()->m_pController->OnLogout(this, m_AccountLogoutQueryResult->m_aMessage);
		}
		m_AccountLogoutQueryResult = nullptr;
	}

	RainbowTick();
}

void CPlayer::RainbowTick()
{
	if(!GetCharacter())
		return;
	if(!GetCharacter()->HasRainbow())
		return;

	m_TeeInfos.m_UseCustomColor = true;
	m_RainbowColor = (m_RainbowColor + 1) % 256;
	m_TeeInfos.m_ColorBody = m_RainbowColor * 0x010000 + 0xff00;
	m_TeeInfos.m_ColorFeet = m_RainbowColor * 0x010000 + 0xff00;

	// ratelimit the 0.7 stuff because it requires net messages
	if(Server()->Tick() % 4 != 0)
		return;

	m_TeeInfos.ToSixup();

	protocol7::CNetMsg_Sv_SkinChange Msg;
	Msg.m_ClientId = GetCid();
	for(int p = 0; p < protocol7::NUM_SKINPARTS; p++)
	{
		Msg.m_apSkinPartNames[p] = m_TeeInfos.m_apSkinPartNames[p];
		Msg.m_aSkinPartColors[p] = m_TeeInfos.m_aSkinPartColors[p];
		Msg.m_aUseCustomColors[p] = m_TeeInfos.m_aUseCustomColors[p];
	}

	for(CPlayer *pRainbowReceiverPlayer : GameServer()->m_apPlayers)
	{
		if(!pRainbowReceiverPlayer)
			continue;
		if(!Server()->IsSixup(pRainbowReceiverPlayer->GetCid()))
			continue;

		const bool IsTopscorer = !GameServer()->m_pController->IsTeamPlay() && GameServer()->m_pController->HasWinningScore(this);

		// never clip when in scoreboard or the top scorer
		// to see the rainbow in scoreboard and hud in the bottom right
		if(!(pRainbowReceiverPlayer->m_PlayerFlags & PLAYERFLAG_SCOREBOARD) && !IsTopscorer)
			if(NetworkClipped(GameServer(), pRainbowReceiverPlayer->GetCid(), GetCharacter()->GetPos()))
				continue;

		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, pRainbowReceiverPlayer->GetCid());
	}
}

void CPlayer::ProcessStatsResult(CInstaSqlResult &Result)
{
	if(Result.m_Success) // SQL request was successful
	{
		switch(Result.m_MessageKind)
		{
		case EInstaSqlRequestType::DIRECT:
			for(auto &aMessage : Result.m_aaMessages)
			{
				if(aMessage[0] == 0)
					break;
				GameServer()->SendChatTarget(m_ClientId, aMessage);
			}
			break;
		case EInstaSqlRequestType::ALL:
		{
			bool PrimaryMessage = true;
			for(auto &aMessage : Result.m_aaMessages)
			{
				if(aMessage[0] == 0)
					break;

				if(GameServer()->ProcessSpamProtection(m_ClientId) && PrimaryMessage)
					break;

				GameServer()->SendChat(-1, TEAM_ALL, aMessage, -1);
				PrimaryMessage = false;
			}
			break;
		}
		case EInstaSqlRequestType::BROADCAST:
			if(Result.m_aBroadcast[0] != 0)
				GameServer()->SendBroadcast(Result.m_aBroadcast, -1);
			break;
		case EInstaSqlRequestType::CHAT_CMD_STATSALL:
			GameServer()->m_pController->OnShowStatsAll(&Result.m_Stats, this, Result.m_Info.m_aRequestedPlayer);
			break;
		case EInstaSqlRequestType::CHAT_CMD_RANK:
			GameServer()->m_pController->OnShowRank(Result.m_Rank, Result.m_RankedScore, Result.m_aRankColumnDisplay, this, Result.m_Info.m_aRequestedPlayer);
			break;
		case EInstaSqlRequestType::CHAT_CMD_MULTIS:
			GameServer()->m_pController->OnShowMultis(&Result.m_Stats, this, Result.m_Info.m_aRequestedPlayer);
			break;
		case EInstaSqlRequestType::CHAT_CMD_STEALS:
			GameServer()->m_pController->OnShowSteals(&Result.m_Stats, this, Result.m_Info.m_aRequestedPlayer);
			break;
		case EInstaSqlRequestType::PLAYER_DATA:
			GameServer()->m_pController->OnLoadedNameStats(&Result.m_Stats, this);
			break;
		}
	}
}

void CPlayer::ProcessAccountResult(CAccountPlayerResult &Result)
{
	if(!Result.m_Success)
	{
		GameServer()->SendChatTarget(GetCid(), "Something went wrong. Please contact an administrator.");
		return;
	}

	switch(Result.m_MessageKind)
	{
	case EAccountPlayerRequestType::LOG_INFO:
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			log_info("ddnet-insta", "%s", aMessage);
		}
		break;
	case EAccountPlayerRequestType::LOG_ERROR:
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			log_error("ddnet-insta", "%s", aMessage);
		}
		break;
	case EAccountPlayerRequestType::CHAT_CMD_SLOW_ACCOUNT_OPERATION:
	case EAccountPlayerRequestType::DIRECT:
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;
			GameServer()->SendChatTarget(m_ClientId, aMessage);
		}
		break;
	case EAccountPlayerRequestType::ALL:
	{
		bool PrimaryMessage = true;
		for(auto &aMessage : Result.m_aaMessages)
		{
			if(aMessage[0] == 0)
				break;

			if(GameServer()->ProcessSpamProtection(m_ClientId) && PrimaryMessage)
				break;

			GameServer()->SendChat(-1, TEAM_ALL, aMessage, -1);
			PrimaryMessage = false;
		}
		break;
	}
	case EAccountPlayerRequestType::BROADCAST:
		if(Result.m_aBroadcast[0] != 0)
			GameServer()->SendBroadcast(Result.m_aBroadcast, -1);
		break;
	case EAccountPlayerRequestType::CHAT_CMD_REGISTER:
		GameServer()->m_pController->OnRegister(this);
		break;
	case EAccountPlayerRequestType::CHAT_CMD_LOGIN:
		GameServer()->m_pController->OnLogin(&Result.m_Account, this);
		break;
	case EAccountPlayerRequestType::CHAT_CMD_CHANGE_PASSWORD:
		GameServer()->m_pController->OnChangePassword(this);
		break;
	case EAccountPlayerRequestType::LOGIN_FAILED:
		GameServer()->m_pController->OnFailedAccountLogin(this, Result.m_aaMessages[0]);
		break;
	}
}

int64_t CPlayer::HandleMulti()
{
	int64_t TimeNow = time_timestamp();
	if((TimeNow - m_LastKillTime) > 5)
	{
		m_Multi = 1;
		return TimeNow;
	}

	if(!GameServer()->m_pController->IsStatTrack())
		return TimeNow;

	m_Multi++;
	if(m_Stats.m_BestMulti < m_Multi)
		m_Stats.m_BestMulti = m_Multi;
	int Index = m_Multi - 2;
	m_Stats.m_aMultis[Index > MAX_MULTIS ? MAX_MULTIS : Index]++;
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' multi x%d!",
		Server()->ClientName(GetCid()), m_Multi);
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);
	return TimeNow;
}

void CPlayer::SetTeamSpoofed(int Team, bool DoChatMsg)
{
	KillCharacter();

	m_Team = Team;
	m_LastSetTeam = Server()->Tick();
	m_LastActionTick = Server()->Tick();

	// TODO: revist this when ddnet merged 128 player support
	//       do we really want to rebuild and resend some 0.6 backcompat mappings here?
	SetSpectatorId(SPEC_FREEVIEW);

	protocol7::CNetMsg_Sv_Team Msg;
	Msg.m_ClientId = m_ClientId;
	Msg.m_Team = GameServer()->m_pController->GetPlayerTeam(this, true); // might be a fake team
	Msg.m_Silent = !DoChatMsg;
	Msg.m_CooldownTick = m_LastSetTeam + Server()->TickSpeed() * g_Config.m_SvTeamChangeDelay;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);

	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(auto &pPlayer : GameServer()->m_apPlayers)
		{
			// TODO: revist this when ddnet merged 128 player support
			if(pPlayer && pPlayer->SpectatorId() == m_ClientId)
				pPlayer->SetSpectatorId(SPEC_FREEVIEW);
		}
	}

	Server()->ExpireServerInfo();
}

void CPlayer::SetTeamNoKill(int Team, bool DoChatMsg)
{
	int OldTeam = m_Team;
	m_Team = Team;
	m_LastSetTeam = Server()->Tick();
	m_LastActionTick = Server()->Tick();
	// TODO: revist this when ddnet merged 128 player support
	SetSpectatorId(SPEC_FREEVIEW);

	// dead spec mode for 0.7
	if(!m_IsDead)
	{
		protocol7::CNetMsg_Sv_Team Msg;
		Msg.m_ClientId = m_ClientId;
		Msg.m_Team = m_Team;
		Msg.m_Silent = !DoChatMsg;
		Msg.m_CooldownTick = m_LastSetTeam + Server()->TickSpeed() * g_Config.m_SvTeamChangeDelay;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, -1);
	}

	// we got to wait 0.5 secs before respawning
	m_RespawnTick = Server()->Tick() + Server()->TickSpeed() / 2;

	if(Team == TEAM_SPECTATORS)
	{
		// update spectator modes
		for(auto &pPlayer : GameServer()->m_apPlayers)
		{
			// TODO: revist this when ddnet merged 128 player support
			if(pPlayer && pPlayer->SpectatorId() == m_ClientId)
				pPlayer->SetSpectatorId(SPEC_FREEVIEW);
		}
	}

	if(OldTeam != TEAM_SPECTATORS)
	{
		if(GameServer()->GetDDRaceTeam(GetCid()) == 0)
			--GameServer()->m_pController->m_aTeamSize[OldTeam];
	}
	if(Team != TEAM_SPECTATORS)
	{
		if(GameServer()->GetDDRaceTeam(GetCid()) == 0)
			++GameServer()->m_pController->m_aTeamSize[Team];
	}

	Server()->ExpireServerInfo();
}

void CPlayer::SetTeamRaw(int Team)
{
	int OldTeam = m_Team;
	if(OldTeam != TEAM_SPECTATORS)
	{
		if(GameServer()->GetDDRaceTeam(GetCid()) == 0)
			--GameServer()->m_pController->m_aTeamSize[OldTeam];
	}
	if(Team != TEAM_SPECTATORS)
	{
		if(GameServer()->GetDDRaceTeam(GetCid()) == 0)
			++GameServer()->m_pController->m_aTeamSize[Team];
	}

	m_Team = Team;
}

void CPlayer::UpdateLastToucher(int ClientId)
{
	if(ClientId == GetCid())
		return;
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
	{
		// covers the reset case when -1 is passed explicitly
		// to reset the last toucher when being hammered by a team mate in fng
		m_LastToucherId = -1;
		return;
	}

	// TODO: should we really reset the last toucher when we get shot by a team mate?
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(
		pPlayer &&
		GameServer()->m_pController &&
		GameServer()->m_pController->IsTeamPlay() &&
		pPlayer->GetTeam() == GetTeam())
	{
		m_LastToucherId = -1;
		return;
	}

	m_LastToucherId = ClientId;
	m_TicksSinceLastTouch = 0;
}
