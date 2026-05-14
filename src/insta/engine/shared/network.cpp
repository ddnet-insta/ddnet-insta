#include <engine/shared/network.h>

void CNetConnection::OccupySlot()
{
	m_State = EState::OCCUPIED;
}

void CNetConnection::FreeOccupiedSlot()
{
	m_State = EState::OFFLINE;
}

void CNetServer::OccupySlot(int ClientId)
{
	if(ClientId >= NET_MAX_CLIENTS)
		return;

	m_aSlots[ClientId].m_Connection.OccupySlot();
}

void CNetServer::FreeOccupiedSlot(int ClientId)
{
	if(ClientId >= NET_MAX_CLIENTS)
		return;

	m_aSlots[ClientId].m_Connection.FreeOccupiedSlot();
}
