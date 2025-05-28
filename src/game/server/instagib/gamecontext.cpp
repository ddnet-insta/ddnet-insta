#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>

bool CGameContext::OnClientPacket(int ClientId, bool Sys, int MsgId, class CNetChunk *pPacket)
{
	if(!m_pController)
		return false;

	return m_pController->OnClientPacket(ClientId, Sys, MsgId, pPacket);
}
