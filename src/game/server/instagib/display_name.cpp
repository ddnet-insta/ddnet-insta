#include "display_name.h"

CDisplayName::CDisplayName()
{
	m_aWantedName[0] = '\0';
	m_aLastBroadcastedName[0] = '\0';
	m_aNameOwner[0] = '\0';
	m_aUsername[0] = '\0';
	m_aCurrentDisplayName[0] = '\0';
}

void CDisplayName::CheckUpdateDisplayName()
{
	if(m_GotNameOwnerResponse)
		m_IsClaimed = m_aNameOwner[0] != '\0';

	m_CanUseName = !m_IsClaimed || !str_comp(m_aNameOwner, m_aUsername);

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
	m_aNameOwner[0] = '\0';
	m_CanUseName = false;
	m_IsClaimed = true;
	m_GotNameOwnerResponse = false;
}

void CDisplayName::SetNameOwner(const char *pUsername)
{
	m_GotNameOwnerResponse = true;
	str_copy(m_aNameOwner, pUsername);
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
