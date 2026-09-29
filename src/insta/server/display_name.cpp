#include "display_name.h"

#include <base/str.h>

CDisplayName::CDisplayName()
{
	m_aWantedName[0] = '\0';
	m_aLastBroadcastedName[0] = '\0';
	m_Owner.Reset();
	m_aUsername[0] = '\0';
	m_aCurrentDisplayName[0] = '\0';
}

void CDisplayName::CheckUpdateDisplayName()
{
	if(m_GotNameOwnerResponse)
		m_IsClaimed = m_Owner.m_aUsername[0] != '\0';

	m_CanUseName =
		!m_IsClaimed ||
		!m_Owner.m_IsProtected ||
		!str_comp(m_Owner.m_aUsername, m_aUsername);

	if(m_CanUseName)
		str_copy(m_aCurrentDisplayName, m_aWantedName);
	else
		str_format(m_aCurrentDisplayName, sizeof(m_aCurrentDisplayName), "(..) %s", m_aWantedName);
}

void CDisplayName::SetAccountUsername(const char *pUsername)
{
	str_copy(m_aUsername, pUsername);
	CheckUpdateDisplayName();
}

void CDisplayName::SetWantedName(const char *pDisplayName)
{
	if(!str_comp(m_aWantedName, pDisplayName))
		return;

	str_copy(m_aWantedName, pDisplayName);
	str_format(m_aCurrentDisplayName, sizeof(m_aCurrentDisplayName), "(..) %s", pDisplayName);

	m_NumChanges++;

	// force that we can not use that newly wanted name
	// we have to wait until the owner is fetched
	// and SetNameOwner is called
	m_Owner.Reset();
	m_CanUseName = false;
	m_IsClaimed = true;
	m_GotNameOwnerResponse = false;
}

void CDisplayName::SetNameOwner(const CDisplayNameOwner *pOwner)
{
	m_GotNameOwnerResponse = true;
	m_Owner = *pOwner;
	CheckUpdateDisplayName();
}

void CDisplayName::SetLastBroadcastedName(const char *pDisplayName)
{
	str_copy(m_aLastBroadcastedName, pDisplayName);
}

const char *CDisplayName::WantedName()
{
	return m_aWantedName;
}

const char *CDisplayName::LastBroadcastedName()
{
	return m_aLastBroadcastedName;
}

const char *CDisplayName::DisplayName()
{
	return m_aCurrentDisplayName;
}

bool CDisplayName::CanUseName() const
{
	return m_CanUseName;
}
