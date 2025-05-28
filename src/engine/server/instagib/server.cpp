#include <base/log.h>
#include <base/system.h>
#include <engine/server/server.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>

void CServer::AddMapToRandomPool(const char *pMap)
{
	m_vMapPool.emplace_back(pMap);
}

void CServer::ClearRandomMapPool()
{
	m_vMapPool.clear();
}

const char *CServer::GetRandomMapFromPool()
{
	if(m_vMapPool.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "map pool is empty add one with 'add_map_to_pool [map name]'");
		return "";
	}

	int RandIdx = secure_rand() % m_vMapPool.size();
	const char *pMap = m_vMapPool[RandIdx].c_str();

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "Chose random map '%s' out of %zu maps", pMap, m_vMapPool.size());
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", aBuf);
	return pMap;
}

void CServer::ConRedirect(IConsole::IResult *pResult, void *pUser)
{
	CServer *pThis = (CServer *)pUser;
	char aBuf[512];

	int VictimId = pResult->GetVictim();
	int Port = pResult->GetInteger(1);

	if(VictimId == pResult->m_ClientId)
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", "You can not redirect your self");
		return;
	}

	if(VictimId < 0 || VictimId >= MAX_CLIENTS)
	{
		str_format(aBuf, sizeof(aBuf), "Invalid ClientId %d", VictimId);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ddnet-insta", aBuf);
		return;
	}
	if(!pThis->ClientIngame(VictimId))
	{
		return;
	}
	pThis->RedirectClient(VictimId, Port);
}

bool CServer::SixupUsernameAuth(int ClientId, const char *pCredentials)
{
	char aName[1024];
	const char *pPw = "";
	str_copy(aName, pCredentials);
	bool FoundSep = false;
	const int StrLen = str_length(aName);
	for(int i = 0; i < StrLen; i++)
	{
		if(aName[i] == ':')
		{
			if(i == 0)
				return false;

			FoundSep = true;
			pPw = aName + i + 1;
			aName[i] = '\0';
			break;
		}
	}
	if(!FoundSep)
		return false;
	if(pPw[0] == '\0')
		return false;
	if(aName[0] == '\0')
		return false;

	int AuthLevel = -1;
	int KeySlot = -1;

	KeySlot = m_AuthManager.FindKey(aName);
	if(m_AuthManager.CheckKey(KeySlot, pPw))
		AuthLevel = m_AuthManager.KeyLevel(KeySlot);

	if(AuthLevel == -1)
		return false;
	if(m_aClients[ClientId].m_Authed == AuthLevel)
		return false;

	CMsgPacker Msgp(protocol7::NETMSG_RCON_AUTH_ON, true, true);
	SendMsg(&Msgp, MSGFLAG_VITAL, ClientId);

	m_aClients[ClientId].m_Authed = AuthLevel; // Keeping m_Authed around is unwise...
	m_aClients[ClientId].m_AuthKey = KeySlot;
	m_aClients[ClientId].m_pRconCmdToSend = Console()->FirstCommandInfo(m_aClients[ClientId].ConsoleAccessLevel(), CFGFLAG_SERVER);
	SendRconCmdGroupStart(ClientId);
	if(m_aClients[ClientId].m_pRconCmdToSend == nullptr)
	{
		SendRconCmdGroupEnd(ClientId);
	}

	char aBuf[256];
	const char *pIdent = m_AuthManager.KeyIdent(KeySlot);
	switch(AuthLevel)
	{
	case AUTHED_ADMIN:
	{
		SendRconLine(ClientId, "Admin authentication successful. Full remote console access granted.");
		str_format(aBuf, sizeof(aBuf), "ClientId=%d authed with key=%s (admin)", ClientId, pIdent);
		break;
	}
	case AUTHED_MOD:
	{
		SendRconLine(ClientId, "Moderator authentication successful. Limited remote console access granted.");
		str_format(aBuf, sizeof(aBuf), "ClientId=%d authed with key=%s (moderator)", ClientId, pIdent);
		break;
	}
	case AUTHED_HELPER:
	{
		SendRconLine(ClientId, "Helper authentication successful. Limited remote console access granted.");
		str_format(aBuf, sizeof(aBuf), "ClientId=%d authed with key=%s (helper)", ClientId, pIdent);
		break;
	}
	}
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "server", aBuf);

	// DDRace
	GameServer()->OnSetAuthed(ClientId, AuthLevel);
	return true;
}
