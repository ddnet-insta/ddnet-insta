#include <base/log.h>
#include <base/system.h>
#include <base/types.h>
#include <engine/shared/config.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/instagib/ip_storage.h>
#include <game/server/instagib/protocol.h>
#include <game/server/player.h>

void CGameContext::OnInitInstagib()
{
	UpdateVoteCheckboxes(); // ddnet-insta
	AlertOnSpecialInstagibConfigs(); // ddnet-insta
	ShowCurrentInstagibConfigsMotd(); // ddnet-insta

	// https://github.com/ddnet-insta/ddnet-insta/issues/341
	if(!str_to_display_score(g_Config.m_SvDisplayScore, &m_DisplayScore))
		log_warn("ddnet-insta", "'%s' is not a valid display score pick one of those: " DISPLAY_SCORE_VALUES, g_Config.m_SvDisplayScore);

	m_pHttp = Kernel()->RequestInterface<IHttp>();

	m_pController->OnInit();
	m_pController->OnRoundStart();
}

void CGameContext::AlertOnSpecialInstagibConfigs(int ClientId) const
{
	if(g_Config.m_SvTournament)
	{
		SendChatTarget(ClientId, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		SendChatTarget(ClientId, "THERE IS A TOURNAMENT IN PROGRESS RIGHT NOW");
		SendChatTarget(ClientId, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		if(g_Config.m_SvTournamentWelcomeChat[0])
			SendChatTarget(ClientId, g_Config.m_SvTournamentWelcomeChat);
	}
	if(g_Config.m_SvOnlyHookKills)
		SendChatTarget(ClientId, "WARNING: only hooked enemies can be killed");
	if(g_Config.m_SvKillHook)
		SendChatTarget(ClientId, "WARNING: the hook kills");
	if(g_Config.m_SvOnlyWallshotKills)
		SendChatTarget(ClientId, "WARNING: only wallshots can kill");
	if(m_pController->IsInfiniteWarmup())
		SendChatTarget(ClientId, "This is a warmup game until a restart vote is called.");
}

void CGameContext::ShowCurrentInstagibConfigsMotd(int ClientId, bool Force) const
{
	if(!g_Config.m_SvShowSettingsMotd && !Force)
		return;
	// should only happen in edge cases
	// is here to avoid crashes
	if(!m_pController)
		return;

	char aMotd[2048];
	char aBuf[512];
	aMotd[0] = '\0';

	if(g_Config.m_SvTournament)
		str_append(aMotd, "!!! ACTIVE TOURNAMENT RUNNING !!!");

	if(g_Config.m_SvPlayerReadyMode)
	{
		str_append(aMotd, "* ready mode: on\n");
		str_format(aBuf, sizeof(aBuf), "  - max pause time: %d\n", g_Config.m_SvForceReadyAll);
		str_append(aMotd, aBuf);
	}
	else
		str_append(aMotd, "* ready mode: off\n");

	// TODO: check if the spawn weapons include laser and only then print this
	if(g_Config.m_SvReloadTimeOnHit > 0)
	{
		str_append(aMotd, "* laser kill refills ammo (on fire mode)\n");
	}

	if(m_pController->GameFlags() & GAMEFLAG_FLAGS)
	{
		if(g_Config.m_SvDropFlagOnVote || g_Config.m_SvDropFlagOnSelfkill)
		{
			str_append(aMotd, "* dropping the flag is on '/drop flag'\n");
			if(g_Config.m_SvDropFlagOnSelfkill)
				str_append(aMotd, "  - selfkill drops the flag\n");
			if(g_Config.m_SvDropFlagOnVote)
				str_append(aMotd, "  - vote yes drops the flag\n");
		}
	}

	str_format(aBuf, sizeof(aBuf), "* allow spec public chat: %s\n", g_Config.m_SvTournamentChat ? "no" : "yes");
	str_append(aMotd, aBuf);

	if(g_Config.m_SvGametype[0] == 'g')
	{
		str_format(aBuf, sizeof(aBuf), "* damage needed for kill: %d\n", g_Config.m_SvDamageNeededForKill);
		str_append(aMotd, aBuf);

		str_format(aBuf, sizeof(aBuf), "* spray protection: %s\n", g_Config.m_SvSprayprotection ? "on" : "off");
		str_append(aMotd, aBuf);
		str_format(aBuf, sizeof(aBuf), "* spam protection: %s\n", g_Config.m_SvGrenadeAmmoRegen ? "on" : "off");
		str_append(aMotd, aBuf);
		if(g_Config.m_SvGrenadeAmmoRegen)
		{
			const char *pMode = "off";
			if(g_Config.m_SvGrenadeAmmoRegenOnKill == 1)
				pMode = "one";
			else if(g_Config.m_SvGrenadeAmmoRegenOnKill == 2)
				pMode = "all";
			str_format(aBuf, sizeof(aBuf), "  - refill nades on kill: %s\n", pMode);
			str_append(aMotd, aBuf);
		}
	}

	// the hammer hitting through walls was a bug in 0.5
	// it got fixed in teeworlds 0.6
	// and was reintroduced by ddnet because its part of the core ddrace gameplay
	// ddnet-insta is based on ddnet and did not revert that back
	// also I (ChillerDragon) personally think the wallhammer was cool and
	// fits into the CTF/DM gameplay
	// see https://github.com/teeworlds/teeworlds/issues/1699
	//
	// because we are not correctly implementing vanilla physics that should be noted
	if(m_pController && m_pController->IsVanillaGameType())
		str_append(aMotd, "* hammer through walls: on\n");

	if(m_pController->IsFngGameType())
	{
		str_format(aBuf, sizeof(aBuf), "* fng hammer tuning: %s\n", g_Config.m_SvFngHammer ? "on" : "off");
		str_append(aMotd, aBuf);
	}
	else if(g_Config.m_SvFngHammer)
	{
		str_append(aMotd, "! WARNING: fng hammer tuning: on\n");
	}

	if(g_Config.m_SvSwapFlags)
		str_append(aMotd, "! WARNING: flag spawns are swapped\n");
	if(g_Config.m_SvAllowZoom)
		str_append(aMotd, "! WARNING: using zoom is allowed\n");
	if(g_Config.m_SvOnlyHookKills)
		str_append(aMotd, "! WARNING: only hooked enemies can be killed\n");
	if(g_Config.m_SvOnlyWallshotKills)
		str_append(aMotd, "! WARNING: only wallshots can kill\n");
	if(g_Config.m_SvKillHook)
		str_append(aMotd, "! WARNING: the hook kills\n");

	CNetMsg_Sv_Motd Msg;
	Msg.m_pMessage = aMotd;
	Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, ClientId);
}

void CGameContext::UpdateVoteCheckboxes() const
{
	if(!g_Config.m_SvVoteCheckboxes)
		return;

	CVoteOptionServer *pCurrent = m_pVoteOptionFirst;
	while(pCurrent != NULL)
	{
		if(str_startswith(pCurrent->m_aDescription, "[ ]") || str_startswith(pCurrent->m_aDescription, "[x]"))
		{
			bool Checked = false;
			int Len;
			int Val;

			if(str_startswith(pCurrent->m_aCommand, "sv_gametype "))
			{
				const char *pVal = pCurrent->m_aCommand + str_length("sv_gametype ");
				Checked = str_startswith_nocase(pVal, g_Config.m_SvGametype);
			}
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Flags, Desc) \
	else if(str_startswith(pCurrent->m_aCommand, #ScriptName) && pCurrent->m_aCommand[str_length(#ScriptName)] == ' ') \
	{ \
		Len = str_length(#ScriptName); \
		/* \
		votes can directly match the command or have other commands \
		or only start with it but then they should be delimited with a semicolon \
		this allows to detect config option votes that also run additonal commands on vote pass \
		*/ \
		if(pCurrent->m_aCommand[Len] != ';' && pCurrent->m_aCommand[Len] != '\0') \
		{ \
			Val = atoi(pCurrent->m_aCommand + Len + 1); \
			Checked = g_Config.m_##Name == Val; \
		} \
	}
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Flags, Desc) // only int checkboxes for now
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Flags, Desc) // only int checkboxes for now
#include <engine/shared/variables_insta.h>
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_COL
#undef MACRO_CONFIG_STR

			pCurrent->m_aDescription[1] = Checked ? 'x' : ' ';
		}
		pCurrent = pCurrent->m_pNext;
	}
	ShowCurrentInstagibConfigsMotd();
}

void CGameContext::RefreshVotes()
{
	// start reloading vote option list
	// clear vote options
	CNetMsg_Sv_VoteClearOptions VoteClearOptionsMsg;
	Server()->SendPackMsg(&VoteClearOptionsMsg, MSGFLAG_VITAL, -1);

	// reset sending of vote options
	for(auto &pPlayer : m_apPlayers)
		if(pPlayer)
			pPlayer->m_SendVoteIndex = 0;
}

void CGameContext::SendBroadcastSix(const char *pText, bool Important)
{
	for(CPlayer *pPlayer : m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(Server()->IsSixup(pPlayer->GetCid()))
			continue;

		// not very nice but the best hack that comes to my mind
		// the broadcast is not rendered if the scoreboard is shown
		// the client shows the scorebaord if he is dead
		// and if there is sv_warmup(igs countdown) in the beginning of a round
		// the chracter did not spawn yet. And thus the server forces a scoreboard
		// on the client. And 0.6 clients just get stuck in that screen without
		// even noticing that a sv_warmup(igs countdown) is happenin.
		//
		// could also abuse the 0.6 warmup timer for that
		// but this causes confusion since a sv_warmup(igs countdown) is not a warmup
		// it is a countdown until the game begins and is a different thing already
		//
		// this hack can be put behind a ddnet client version check if
		// https://github.com/ddnet/ddnet/pull/8892
		// is merged. If it is merged for long enough the entire ghost char hack can be removed
		// and we just drop broadcast support for old clients in that weird state
		if(!pPlayer->m_HasGhostCharInGame && pPlayer->GetTeam() != TEAM_SPECTATORS)
		{
			if(Server()->GetClientVersion(pPlayer->GetCid()) < VERSION_DDNET_NO_SCOREBOARD_DURING_PAUSE)
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s (update your client to hide this message)", pText);
				SendChatTarget(pPlayer->GetCid(), aBuf);
			}
		}

		SendBroadcast(pText, pPlayer->GetCid(), Important);
	}
}

void CGameContext::PlayerReadyStateBroadcast()
{
	if(!Config()->m_SvPlayerReadyMode)
		return;
	// if someone presses ready change during countdown
	// we ignore it
	if(m_pController->IsGameCountdown())
		return;

	int NumUnready = 0;
	m_pController->GetPlayersReadyState(-1, &NumUnready);

	if(!NumUnready)
		return;

	// TODO: create Sendbroadcast sixup
	//       better pr a sixup flag upstream for sendbroadcast
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "%d players not ready", NumUnready);
	SendBroadcastSix(aBuf, false);
}

void CGameContext::SendGameMsg(int GameMsgId, int ClientId) const
{
	dbg_assert(
		GameMsgId == protocol7::GAMEMSG_TEAM_SWAP ||
			GameMsgId == protocol7::GAMEMSG_SPEC_INVALIDID ||
			GameMsgId == protocol7::GAMEMSG_TEAM_SHUFFLE ||
			GameMsgId == protocol7::GAMEMSG_TEAM_BALANCE ||
			GameMsgId == protocol7::GAMEMSG_CTF_DROP ||
			GameMsgId == protocol7::GAMEMSG_CTF_RETURN,
		"the passed game message id does not take 0 arguments");

	CMsgPacker Msg(protocol7::NETMSGTYPE_SV_GAMEMSG, false, true);
	Msg.AddInt(GameMsgId);
	if(ClientId != -1 && Server()->IsSixup(ClientId))
	{
		Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientId);
		return;
	}
	for(int i = 0; i < Server()->MaxClients(); i++)
	{
		if(Server()->IsSixup(i))
		{
			Server()->SendMsg(&Msg, MSGFLAG_VITAL, i);
			continue;
		}
		// TODO: 0.6
	}
}

void CGameContext::SendGameMsg(int GameMsgId, int ParaI1, int ClientId) const
{
	dbg_assert(
		GameMsgId == protocol7::GAMEMSG_TEAM_ALL ||
			GameMsgId == protocol7::GAMEMSG_TEAM_BALANCE_VICTIM ||
			GameMsgId == protocol7::GAMEMSG_CTF_GRAB ||
			GameMsgId == protocol7::GAMEMSG_GAME_PAUSED,
		"the passed game message id does not take 1 argument");

	CMsgPacker Msg(protocol7::NETMSGTYPE_SV_GAMEMSG, false, true);
	Msg.AddInt(GameMsgId);
	Msg.AddInt(ParaI1);
	if(ClientId != -1 && Server()->IsSixup(ClientId))
	{
		Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientId);
		return;
	}
	for(int i = 0; i < Server()->MaxClients(); i++)
	{
		if(!m_apPlayers[i])
			continue;

		if(Server()->IsSixup(i))
		{
			Server()->SendMsg(&Msg, MSGFLAG_VITAL, i);
			continue;
		}
		if(GameMsgId == protocol7::GAMEMSG_GAME_PAUSED)
		{
			char aBuf[512];
			int PauseId = std::clamp(ParaI1, 0, Server()->MaxClients() - 1);
			str_format(aBuf, sizeof(aBuf), "'%s' initiated a pause. If you are ready do /ready", Server()->ClientName(PauseId));
			SendChatTarget(i, aBuf);
		}
	}
}

void CGameContext::SendGameMsg(int GameMsgId, int ParaI1, int ParaI2, int ParaI3, int ClientId) const
{
	dbg_assert(GameMsgId == protocol7::GAMEMSG_CTF_CAPTURE, "the passed game message id does not take 3 arguments");

	CMsgPacker Msg(protocol7::NETMSGTYPE_SV_GAMEMSG, false, true);
	Msg.AddInt(GameMsgId);
	Msg.AddInt(ParaI1);
	Msg.AddInt(ParaI2);
	Msg.AddInt(ParaI3);
	if(ClientId != -1)
		Server()->SendMsg(&Msg, MSGFLAG_VITAL, ClientId);
	for(int i = 0; i < Server()->MaxClients(); i++)
	{
		if(Server()->IsSixup(i))
		{
			Server()->SendMsg(&Msg, MSGFLAG_VITAL, i);
			continue;
		}
		// TODO: 0.6
	}
}

void CGameContext::InstagibUnstackChatMessage(char *pUnstacked, const char *pMessage, int Size)
{
	bool Match = false;
	for(const char *aLastChatMessage : m_aaLastChatMessages)
	{
		if(!str_comp(aLastChatMessage, pMessage))
		{
			Match = true;
			break;
		}
	}
	if(Match)
	{
		char aaInvisibleUnicodes[][8] = {
			"", // no unicode is also different than unicode :D
			"‌", // U+200C ZERO WIDTH NON-JOINER
			"‍", // U+200D ZERO WIDTH JOINER
			"‌", // U+200E LEFT-TO-RIGHT MARK
			"‎", // U+200F RIGHT-TO-LEFT MARK
			"⁠", // U+2060 WORD JOINER
			"⁡", // U+2061 FUNCTION APPLICATION
			"⁢", // U+2062 INVISIBLE TIMES
			"⁣", // U+2063 INVISIBLE SEPARATOR
			"⁤" // U+2064 INVISIBLE PLUS
		};
		m_UnstackHackCharacterOffset++;
		if(m_UnstackHackCharacterOffset >= (int)(sizeof(aaInvisibleUnicodes) / 8))
			m_UnstackHackCharacterOffset = 0;

		const char *pPingPrefix = "";

		if(aaInvisibleUnicodes[m_UnstackHackCharacterOffset][0] != '\0')
		{
			for(const CPlayer *pPlayer : m_apPlayers)
			{
				if(!pPlayer)
					continue;

				if(str_startswith(pMessage, Server()->ClientName(pPlayer->GetCid())))
				{
					pPingPrefix = " ";
					break;
				}
			}
		}

		str_format(pUnstacked, Size, "%s%s%s", aaInvisibleUnicodes[m_UnstackHackCharacterOffset], pPingPrefix, pMessage);
	}
	for(int i = MAX_LINES - 1; i > 0; i--)
	{
		str_copy(m_aaLastChatMessages[i], m_aaLastChatMessages[i - 1]);
	}
	str_copy(m_aaLastChatMessages[0], pMessage);
}

void CGameContext::SwapTeams()
{
	if(!m_pController)
		return;
	if(!m_pController->IsTeamPlay())
		return;

	SendGameMsg(protocol7::GAMEMSG_TEAM_SWAP, -1);

	for(CPlayer *pPlayer : m_apPlayers)
	{
		if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS)
			m_pController->DoTeamChange(pPlayer, pPlayer->GetTeam() ^ 1, false);
	}

	m_pController->SwapTeamscore();
}

bool CGameContext::OnClientPacket(int ClientId, bool Sys, int MsgId, CNetChunk *pPacket, CUnpacker *pUnpacker)
{
	if(!m_pController)
		return false;

	return m_pController->OnClientPacket(ClientId, Sys, MsgId, pPacket, pUnpacker);
}

void CGameContext::DeepJailId(int AdminId, int ClientId, int Minutes)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
	{
		log_info("deep_jail", "invalid id %d", ClientId);
		return;
	}
	CPlayer *pPlayer = m_apPlayers[ClientId];
	if(!pPlayer)
	{
		log_info("deep_jail", "no player with id %d found", ClientId);
		return;
	}
	if(Minutes < 1)
	{
		log_info("deep_jail", "minute amount has to be at least 1");
		return;
	}

	int MinutesInTicks = Minutes * Server()->TickSpeed() * 60;
	pPlayer->InitIpStorage();
	pPlayer->m_IpStorage.value().SetDeepUntilTick(Server()->Tick() + MinutesInTicks);
	if(pPlayer->GetCharacter())
	{
		pPlayer->GetCharacter()->SetDeepFrozen(true);
	}

	char aBuf[512];
	if(AdminId < 0 || AdminId >= MAX_CLIENTS)
		str_copy(aBuf, "you were deep frozen by the server", sizeof(aBuf));
	else
		str_format(aBuf, sizeof(aBuf), "you were deep frozen by '%s'", Server()->ClientName(AdminId));
	SendChatTarget(pPlayer->GetCid(), aBuf);
	log_info("deep_jail", "deep froze player '%s' for %d minutes", Server()->ClientName(ClientId), Minutes);
}

void CGameContext::DeepJailIp(int AdminId, const char *pAddrStr, int Minutes)
{
	NETADDR Addr;
	if(net_addr_from_str(&Addr, pAddrStr))
	{
		log_info("deep_jail", "invalid ip address '%s'", pAddrStr);
		return;
	}

	int MinutesInTicks = Minutes * Server()->TickSpeed() * 60;
	int UndeepTick = Server()->Tick() + MinutesInTicks;
	CIpStorage *pEntry = m_IpStorageController.FindOrCreateEntry(&Addr);
	pEntry->SetDeepUntilTick(UndeepTick);

	for(CPlayer *pPlayer : m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(net_addr_comp_noport(Server()->ClientAddr(pPlayer->GetCid()), &Addr))
			continue;

		pPlayer->InitIpStorage();
		pPlayer->m_IpStorage.value().SetDeepUntilTick(UndeepTick);
		if(pPlayer->GetCharacter())
		{
			pPlayer->GetCharacter()->SetDeepFrozen(true);
		}

		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "you were deep frozen by '%s'", Server()->ClientName(AdminId));
		SendChatTarget(pPlayer->GetCid(), aBuf);
		log_info("deep_jail", "deep froze player '%s' for %d minutes", Server()->ClientName(pPlayer->GetCid()), Minutes);
	}
}

void CGameContext::UndeepJail(CIpStorage *pEntry)
{
	pEntry->SetDeepUntilTick(0);
	bool OnlinePlayers = false;
	for(CPlayer *pPlayer : m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(net_addr_comp_noport(Server()->ClientAddr(pPlayer->GetCid()), pEntry->Addr()))
			continue;

		log_info("deep_jail", "undeeped player cid=%d name='%s'", pPlayer->GetCid(), Server()->ClientName(pPlayer->GetCid()));
		if(pPlayer->m_IpStorage.has_value())
			pPlayer->m_IpStorage.value().SetDeepUntilTick(0);

		// This could be used to cheat in ddrace.
		// Does not seem safe. If a admin really wants instant unfreeze they
		// have to also kill the player (or we need some undeep id command).
		if(!g_Config.m_SvTestingCommands)
		{
			log_info("deep_jail", "the player has to kill to be undeeped because sv_test_cmds is 0");
		}
		else if(pPlayer->GetCharacter())
		{
			pPlayer->GetCharacter()->SetDeepFrozen(false);
			pPlayer->GetCharacter()->UnFreeze();
		}
		OnlinePlayers = true;
	}
	if(!OnlinePlayers)
	{
		char aAddr[512];
		net_addr_str(pEntry->Addr(), aAddr, sizeof(aAddr), false);
		log_info("deep_jail", "removed deep jail entry %s", aAddr);
	}
}

void CGameContext::ListDeepJails() const
{
	bool GotEntries = false;
	for(const CIpStorage &Entry : m_IpStorageController.Entries())
	{
		if(Entry.DeepUntilTick() < Server()->Tick())
			continue;

		char aAddr[512];
		net_addr_str(Entry.Addr(), aAddr, sizeof(aAddr), false);

		int TicksLeft = Entry.DeepUntilTick() - Server()->Tick();
		int MinutesLeft = (TicksLeft / Server()->TickSpeed()) / 60;

		log_info("deep_jail", "#%d ip=%s minutes=%d", Entry.EntryId(), aAddr, MinutesLeft);
		GotEntries = true;
	}
	for(CPlayer *pPlayer : m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(!pPlayer->m_IpStorage.has_value())
			continue;
		CIpStorage *pEntry = &pPlayer->m_IpStorage.value();
		if(pEntry->DeepUntilTick() < Server()->Tick())
			continue;

		int TicksLeft = pEntry->DeepUntilTick() - Server()->Tick();
		int MinutesLeft = (TicksLeft / Server()->TickSpeed()) / 60;

		log_info(
			"deep_jail",
			"#%d name='%s' cid=%d minutes=%d",
			pEntry->EntryId(),
			Server()->ClientName(pPlayer->GetCid()),
			pPlayer->GetCid(),
			MinutesLeft);
		GotEntries = true;
	}
	if(!GotEntries)
		log_info("deep_jail", "No entries");
}

CIpStorage *CGameContext::FindIpStorageEntryOfflineAndOnline(int EntryId)
{
	CIpStorage *pEntry = m_IpStorageController.FindEntry(EntryId);
	if(pEntry)
		return pEntry;
	for(CPlayer *pPlayer : m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(!pPlayer->m_IpStorage.has_value())
			continue;
		if(pPlayer->m_IpStorage.value().EntryId() != EntryId)
			continue;

		return &pPlayer->m_IpStorage.value();
	}
	return nullptr;
}

bool CGameContext::IsChatCmdAllowed(int ClientId) const
{
	if(g_Config.m_SvBangCommands < 2)
	{
		SendChatTarget(ClientId, "Chat votes are disabled please use the vote menu.");
		return false;
	}
	return true;
}

bool CGameContext::CanClientDrop(int ClientId, const char *pReason)
{
	return m_pController->CanClientDrop(ClientId, pReason);
}
