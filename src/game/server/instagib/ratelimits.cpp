#include <base/system.h>
#include <engine/shared/protocol.h>

#include "ratelimits.h"

#include <algorithm>

CIpRatelimit *CIpRatelimit::FindLimit(std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr)
{
	for(CIpRatelimit &Limit : vRatelimits)
	{
		if(net_addr_comp_noport(&Limit.m_Addr, pAddr))
			continue;
		return &Limit;
	}
	return nullptr;
}

const CIpRatelimit *CIpRatelimit::FindLimit(const std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr)
{
	for(const CIpRatelimit &Limit : vRatelimits)
	{
		if(net_addr_comp_noport(&Limit.m_Addr, pAddr))
			continue;
		return &Limit;
	}
	return nullptr;
}

bool CIpRatelimit::IsLoginRatelimited(
	const std::vector<CIpRatelimit> &vRatelimits,
	const NETADDR *pAddr)
{
	const CIpRatelimit *pLimit = FindLimit(vRatelimits, pAddr);
	if(!pLimit)
		return false;

	return pLimit->m_WrongAccountLogins > 3;
}

void CIpRatelimit::TrackWrongLogin(std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr, int CurrentServerTick)
{
	CIpRatelimit *pLimit = FindLimit(vRatelimits, pAddr);
	if(pLimit)
	{
		pLimit->m_LastSeenTick = CurrentServerTick;
		pLimit->m_LastWrongLoginTick = CurrentServerTick;
		pLimit->m_WrongAccountLogins++;
		return;
	}

	CIpRatelimit Limit = CIpRatelimit(pAddr, CurrentServerTick);
	Limit.m_LastWrongLoginTick = CurrentServerTick;
	Limit.m_WrongAccountLogins++;
	vRatelimits.emplace_back(Limit);
}

void CIpRatelimit::CheckExpireTick(std::vector<CIpRatelimit> &vRatelimits, int CurrentServerTick)
{
	// expire /login chat command ratelimits
	for(CIpRatelimit &Ratelimit : vRatelimits)
	{
		if(Ratelimit.m_LastWrongLoginTick)
		{
			int MinsSinceLogin = ((CurrentServerTick - Ratelimit.m_LastWrongLoginTick) / SERVER_TICK_SPEED) / 60;
			if(MinsSinceLogin > 5)
			{
				Ratelimit.m_LastWrongLoginTick = 0;
				Ratelimit.m_WrongAccountLogins = 0;
			}
		}
	}

	// remove the entire ip ratelimit entry with all kinds of
	// ratelimits if the ip did not run into any ratelimit for 10 minutes
	vRatelimits.erase(std::remove_if(
				  vRatelimits.begin(), vRatelimits.end(),
				  [CurrentServerTick](const CIpRatelimit &Ratelimit) {
					  int MinsSinceLastSeen = ((CurrentServerTick - Ratelimit.m_LastWrongLoginTick) / SERVER_TICK_SPEED) / 60;
					  return MinsSinceLastSeen > 10;
				  }),
		vRatelimits.end());
}
