#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <game/server/gamecontext.h>
#include <game/server/gamecontroller.h>

bool CGameContext::OnClientPacket(int ClientId, bool Sys, int MsgId, CNetChunk *pPacket, CUnpacker *pUnpacker)
{
	if(!m_pController)
		return false;

	return m_pController->OnClientPacket(ClientId, Sys, MsgId, pPacket, pUnpacker);
}
