#include <engine/shared/config.h>

#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>
#include <game/server/player.h>

void CGameContext::BangCommandVote(int ClientId, const char *pCommand, const char *pDesc)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;
	CPlayer *pPlayer = m_apPlayers[ClientId];
	if(!pPlayer)
		return;
	// not needed for bang command but for slash command
	if(pPlayer->GetTeam() == TEAM_SPECTATORS && !g_Config.m_SvSpectatorVotes)
	{
		SendChatTarget(ClientId, "Spectators aren't allowed to vote.");
		return;
	}
	char aChatmsg[1024];
	str_format(aChatmsg, sizeof(aChatmsg), "'%s' called vote to change server option '%s'", Server()->ClientName(ClientId), pDesc);
	m_VoteType = CGameContext::VOTE_TYPE_OPTION;
	CallVote(ClientId, pDesc, pCommand, "chat cmd", aChatmsg);
}

void CGameContext::ComCallShuffleVote(int ClientId)
{
	BangCommandVote(ClientId, "shuffle_teams", "shuffle teams");
}

void CGameContext::ComCallSwapTeamsVote(int ClientId)
{
	BangCommandVote(ClientId, "swap_teams", "swap teams");
}

void CGameContext::ComCallSwapTeamsRandomVote(int ClientId)
{
	BangCommandVote(ClientId, "swap_teams_random", "swap teams (random)");
}

void CGameContext::ComDropFlag(int ClientId)
{
	CPlayer *pPlayer = m_apPlayers[ClientId];
	if(!pPlayer)
		return;

	if(!g_Config.m_SvDropFlagOnVote && !g_Config.m_SvDropFlagOnSelfkill)
	{
		SendChatTarget(ClientId, "dropping flags is deactivated");
		return;
	}

	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return;

	if(!m_pController)
		return;

	m_pController->DropFlag(pChr);
}
