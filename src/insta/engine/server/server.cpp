#include <base/bytes.h>
#include <base/log.h>
#include <base/secure.h>

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

	int RandIdx = secure_rand_below(m_vMapPool.size());
	const char *pMap = m_vMapPool[RandIdx].c_str();

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "Chose random map '%s' out of %" PRIzu " maps", pMap, m_vMapPool.size());
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

	// only hijack the call and alter the credentials if they work
	// if they don't fallback to regular ddnet code
	// which could match a default key instead of a user key
	int AuthLevel = -1;
	int KeySlot = -1;
	KeySlot = m_AuthManager.FindKey(aName);
	if(m_AuthManager.CheckKey(KeySlot, pPw))
		AuthLevel = m_AuthManager.KeyLevel(KeySlot);
	if(AuthLevel == -1)
		return false;

	OnNetMsgRconAuth(ClientId, aName, pPw, true);
	return true;
}

int CServer::CreateTee(const char *pName)
{
	int ClientId = -1;
	for(int i = 0; i < MaxClients(); i++)
	{
		if(m_aClients[i].m_State != CClient::STATE_EMPTY)
			continue;

		ClientId = i;
		break;
	}
	if(ClientId == -1)
		return ClientId;

	CClient &Client = m_aClients[ClientId];
	NewClientCallback(ClientId, this, false);
	Client.m_DebugDummy = true;

	// See https://en.wikipedia.org/wiki/Unique_local_address
	Client.m_DebugDummyAddr.type = NETTYPE_IPV6;
	Client.m_DebugDummyAddr.ip[0] = 0xfd;
	// Global ID (40 bits): random
	secure_random_fill(&Client.m_DebugDummyAddr.ip[1], 5);
	// Subnet ID (16 bits): constant
	Client.m_DebugDummyAddr.ip[6] = 0xc0;
	Client.m_DebugDummyAddr.ip[7] = 0xde;
	// Interface ID (64 bits): set to client ID
	Client.m_DebugDummyAddr.ip[8] = 0x00;
	Client.m_DebugDummyAddr.ip[9] = 0x00;
	Client.m_DebugDummyAddr.ip[10] = 0x00;
	Client.m_DebugDummyAddr.ip[11] = 0x00;
	uint_to_bytes_be(&Client.m_DebugDummyAddr.ip[12], ClientId);
	// Port: random like normal clients
	Client.m_DebugDummyAddr.port = secure_rand_below(65535 - 1024) + 1024;
	net_addr_str(&Client.m_DebugDummyAddr, Client.m_aDebugDummyAddrString.data(), Client.m_aDebugDummyAddrString.size(), true);
	net_addr_str(&Client.m_DebugDummyAddr, Client.m_aDebugDummyAddrStringNoPort.data(), Client.m_aDebugDummyAddrStringNoPort.size(), false);

	m_NetServer.OccupySlot(ClientId);
	GameServer()->OnClientConnected(ClientId, nullptr);
	Client.m_State = CClient::STATE_INGAME;
	str_copy(Client.m_aName, pName);
	GameServer()->OnClientEnter(ClientId);

	return ClientId;
}

void CServer::DropTee(int ClientId)
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return;
	CClient &Client = m_aClients[ClientId];
	if(!Client.m_DebugDummy)
		return;

	DelClientCallback(ClientId, "", this);
	m_NetServer.FreeOccupiedSlot(ClientId);
}

bool CServer::IsDebugDummy(int ClientId) const
{
	if(ClientId < 0 || ClientId >= MAX_CLIENTS)
		return false;
	return m_aClients[ClientId].m_DebugDummy;
}
