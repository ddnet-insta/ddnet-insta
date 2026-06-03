#include "ratelimits.h"

#include <base/log.h>
#include <base/net.h>
#include <base/str.h>

#include <engine/shared/protocol.h>

#include <algorithm>

namespace Ratelimits
{
	// after how many failed attempts we ratelimit the user from
	// attempting another account login
	// after EXPIRE_WRONG_LOGIN_IN_MINUTES minutes of no failed
	// login attempts this will be reset
	inline constexpr int MAX_WRONG_ACCOUNT_LOGINS = 3;

	// after this many minutes without failed login attempts
	// the login ratelimit will be lifted if it previously got
	// hit by the user doing MAX_WRONG_ACCOUNT_LOGINS failed attempts
	inline constexpr int EXPIRE_WRONG_LOGIN_IN_MINUTES = 5;
}

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

void CIpRatelimit::RconCmdLogOrReset(
	std::vector<CIpRatelimit> &vRatelimits,
	const NETADDR *pAddr,
	int CurrentServerTick,
	const char *pCommand)
{
	char aAddr[NETADDR_MAXSTRSIZE];
	net_addr_str(pAddr, aAddr, sizeof(aAddr), false);

	CIpRatelimit *pLimit = FindLimit(vRatelimits, pAddr);
	if(!pLimit)
	{
		log_info("ratelimit", "the ip %s has not triggered any ratelimits", aAddr);
		return;
	}

	if(pCommand && pCommand[0])
	{
		if(str_comp_nocase(pCommand, "reset") == 0)
		{
			pLimit->m_LastWrongLoginTick = 0;
			pLimit->m_WrongAccountLogins = 0;
			log_info("ratelimit", "reset ratelimit for ip %s", aAddr);
			return;
		}
		else
		{
			log_info("ratelimits", "unsupported argument '%s' possible values: reset", pCommand);
			return;
		}
	}

	bool IsBlocked = pLimit->m_WrongAccountLogins >= Ratelimits::MAX_WRONG_ACCOUNT_LOGINS;
	log_info("ratelimit", "ratelimit stats for ip %s", aAddr);
	log_info("ratelimit", "  can login: %s", IsBlocked ? "NO (blocked)" : "YES");
	if(IsBlocked)
	{
		int MinsSinceLogin = ((CurrentServerTick - pLimit->m_LastWrongLoginTick) / SERVER_TICK_SPEED) / 60;
		int ExpireInMinutes = Ratelimits::EXPIRE_WRONG_LOGIN_IN_MINUTES - MinsSinceLogin;
		log_info("ratelimit", "  block will expire in: %d minutes", ExpireInMinutes);
	}
	log_info("ratelimit", "  wrong logins: %d", pLimit->m_WrongAccountLogins);
	log_info("ratelimit", "  pass 'reset' as additional argument to unblock this user");
}

bool CIpRatelimit::IsLoginRatelimited(
	const std::vector<CIpRatelimit> &vRatelimits,
	const NETADDR *pAddr)
{
	const CIpRatelimit *pLimit = FindLimit(vRatelimits, pAddr);
	if(!pLimit)
		return false;

	return pLimit->m_WrongAccountLogins >= Ratelimits::MAX_WRONG_ACCOUNT_LOGINS;
}

bool CIpRatelimit::TrackWrongLogin(std::vector<CIpRatelimit> &vRatelimits, const NETADDR *pAddr, int CurrentServerTick)
{
	CIpRatelimit *pLimit = FindLimit(vRatelimits, pAddr);
	if(pLimit)
	{
		pLimit->m_LastSeenTick = CurrentServerTick;
		pLimit->m_LastWrongLoginTick = CurrentServerTick;
		pLimit->m_WrongAccountLogins++;
		return pLimit->m_WrongAccountLogins == Ratelimits::MAX_WRONG_ACCOUNT_LOGINS;
	}

	CIpRatelimit Limit = CIpRatelimit(pAddr, CurrentServerTick);
	Limit.m_LastWrongLoginTick = CurrentServerTick;
	Limit.m_WrongAccountLogins++;
	vRatelimits.emplace_back(Limit);
	return false;
}

void CIpRatelimit::CheckExpireTick(std::vector<CIpRatelimit> &vRatelimits, int CurrentServerTick)
{
	// expire /login chat command ratelimits
	for(CIpRatelimit &Ratelimit : vRatelimits)
	{
		if(Ratelimit.m_LastWrongLoginTick)
		{
			int MinsSinceLogin = ((CurrentServerTick - Ratelimit.m_LastWrongLoginTick) / SERVER_TICK_SPEED) / 60;
			if(MinsSinceLogin >= Ratelimits::EXPIRE_WRONG_LOGIN_IN_MINUTES)
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
					  return MinsSinceLastSeen >= 10;
				  }),
		vRatelimits.end());
}
