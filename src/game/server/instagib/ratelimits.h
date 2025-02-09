#ifndef GAME_SERVER_INSTAGIB_RATELIMITS_H
#define GAME_SERVER_INSTAGIB_RATELIMITS_H

#include <vector>

#include <base/types.h>

class CIpRatelimit
{
public:
	CIpRatelimit(const NETADDR *pAddr, int LastSeenTick)
	{
		m_Addr = *pAddr;
		m_LastSeenTick = LastSeenTick;
	};

	NETADDR m_Addr;

	// in which tick this ip did run into
	// any rate limit the last time
	// see also m_LastWrongLoginTick for login specific last seen
	int64_t m_LastSeenTick = 0;

	int64_t m_LastWrongLoginTick = 0;
	int m_WrongAccountLogins = 0;

	static CIpRatelimit *FindLimit(std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr);
	const static CIpRatelimit *FindLimit(const std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr);
	static bool IsLoginRatelimited(const std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr);
	static void TrackWrongLogin(std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr, int CurrentServerTick);

	static void CheckExpireTick(std::vector<CIpRatelimit> &vRatelimits, int CurrentServerTick);
};

#endif
