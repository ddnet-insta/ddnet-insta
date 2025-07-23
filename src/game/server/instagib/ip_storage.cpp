#include <base/system.h>

#include "ip_storage.h"

void CIpStorage::OnInit(const NETADDR *pAddr, int EntryId, uint32_t UniqueClientId)
{
	m_Addr = *pAddr;
	m_EntryId = EntryId;
	m_UniqueClientId = UniqueClientId;
}

CIpStorage::CIpStorage(const NETADDR *pAddr, int EntryId)
{
	OnInit(pAddr, EntryId, -1);
}

bool CIpStorage::IsEmpty(int ServerTick) const
{
	if(m_DeepUntilTick > ServerTick)
		return false;
	return true;
}

void CIpStorage::Merge(const CIpStorage *pOther)
{
	m_DeepUntilTick = std::max(m_DeepUntilTick, pOther->m_DeepUntilTick);
}

void CIpStorage::OnPlayerDisconnect(const CIpStorage *pPlayerSession, int ServerTick)
{
	if(!pPlayerSession->IsEmpty(ServerTick))
		Merge(pPlayerSession);
	m_UniqueClientId = -1;
}

CIpStorage *CIpStorageController::FindEntry(const NETADDR *pAddr)
{
	for(CIpStorage &Entry : m_vEntries)
	{
		if(net_addr_comp_noport(Entry.Addr(), pAddr))
			continue;
		return &Entry;
	}
	return nullptr;
}

CIpStorage *CIpStorageController::FindEntry(int EntryId)
{
	for(CIpStorage &Entry : m_vEntries)
	{
		if(Entry.EntryId() != EntryId)
			continue;
		return &Entry;
	}
	return nullptr;
}

CIpStorage *CIpStorageController::FindOrCreateEntry(const NETADDR *pAddr)
{
	CIpStorage *pEntry = FindEntry(pAddr);
	if(pEntry)
		return pEntry;

	m_vEntries.emplace_back(pAddr, GetNextEntryId());
	pEntry = FindEntry(pAddr);
	dbg_assert(pEntry != nullptr, "failed to find newly created entry");
	return pEntry;
}

void CIpStorageController::OnTick(int ServerTick)
{
	// clear out all empty entries to free the memory
	// empty entries contain only expired storage data or no data at all
	m_vEntries.erase(std::remove_if(
				 m_vEntries.begin(), m_vEntries.end(),
				 [ServerTick](const CIpStorage &Entry) {
					 return Entry.IsEmpty(ServerTick);
				 }),
		m_vEntries.end());
}
