#ifndef GAME_SERVER_INSTAGIB_IP_STORAGE_H
#define GAME_SERVER_INSTAGIB_IP_STORAGE_H

#include <base/types.h>
#include <vector>

// This class tracks player state on an ip address.
// This can be used to avoid losing player state on reconnect.
// It can not be used to match a specific tee. So it will apply
// to all tees using that ip.
// So store only things where restoring the state multiple times is okay.
// The state can be restored for the main player, the dummy and room mates using the same ip.
// This should never be used to restore stats like killing sprees
// or account login state.
class CIpStorage
{
	NETADDR m_Addr;
	int m_EntryId = 0;

	// is the players unique client id
	// if the player is online
	// or -1 if the player is offline
	int m_UniqueClientId = -1;

	int64_t m_DeepUntilTick = 0;

public:
	CIpStorage(const NETADDR *pAddr, int EntryId, uint32_t UniqueClientId);

	const NETADDR *Addr() const { return &m_Addr; };
	int EntryId() const { return m_EntryId; };
	int UniqueClientId() const { return m_UniqueClientId; }
	bool IsEmpty(int ServerTick) const;

	// merge other ip storage entry int our self
	// this is used on disconnect persist the players temporary
	// storage into the permanent storage.
	// Because there might already be a permanent entry from before.
	// Or from another tee with the same ip we have to merge it.
	void Merge(const CIpStorage *pOther);

	void OnPlayerDisconnect(const CIpStorage *pPlayerSession, int ServerTick);

	void SetDeepUntilTick(int ServerTick) { m_DeepUntilTick = ServerTick; }
	int64_t DeepUntilTick() const { return m_DeepUntilTick; }
};

// Manages ip storage instances see also the documentation of
// `CIpStorage` for more details
//
// Manages ip based data storage.
// Does not live sync the data to all connected players!
// It will only write to storage on player disconnect.
// And only load from storage on player connect.
class CIpStorageController
{
	std::vector<CIpStorage> m_vEntries;
	int m_EntryIdCounter = 1;

public:
	int GetNextEntryId() { return m_EntryIdCounter++; }
	const std::vector<CIpStorage> &Entries() const { return m_vEntries; }
	CIpStorage *FindEntry(const NETADDR *pAddr);
	CIpStorage *FindEntry(int EntryId);
	CIpStorage *FindOrCreateEntry(const NETADDR *pAddr);
	void OnTick(int ServerTick);
};

#endif
