#include <base/log.h>
#include <base/system.h>

#include <cstdint>

#include "ip_storage.h"

CIpStorage::CIpStorage(const NETADDR *pAddr, int EntryId, uint32_t UniqueClientId) :
	m_Addr(*pAddr), m_EntryId(EntryId), m_UniqueClientId(UniqueClientId)
{
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

	m_vEntries.emplace_back(pAddr, GetNextEntryId(), -1);
	pEntry = FindEntry(pAddr);
	dbg_assert(pEntry != nullptr, "failed to find newly created entry");
	return pEntry;
}

void CIpStorageController::OnTick(int ServerTick)
{
	bool Deleted = false;
	// clear out all empty entries to free the memory
	// empty entries contain only expired storage data or no data at all
	m_vEntries.erase(std::remove_if(
				 m_vEntries.begin(), m_vEntries.end(),
				 [ServerTick, &Deleted](const CIpStorage &Entry) {
					 if(!Entry.IsEmpty(ServerTick))
						 return false;

					 Deleted = true;
					 char aAddr[512];
					 net_addr_str(Entry.Addr(), aAddr, sizeof(aAddr), false);
					 log_info("ddnet-insta", "ip storage entry expired. ip=%s entryid=%d", aAddr, Entry.EntryId());
					 return true;
				 }),
		m_vEntries.end());

	if(Deleted)
	{
		log_info("ddnet-insta", "ip storage entries after cleanup: %ld", m_vEntries.size());
	}
}
