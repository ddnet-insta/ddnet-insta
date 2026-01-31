#include "skin_info_manager.h"

#include <base/str.h>

#include <game/client/skin.h>
#include <game/server/teeinfo.h>

#include <optional>

bool CSkinOverrideRequest::HasAnyValue()
{
	return m_SkinName.has_value() ||
	       m_ColorBody.has_value() ||
	       m_ColorFeet.has_value() ||
	       m_UseCustomColor.has_value();
}

// CSkinInfoManager::CSkinInfoManager()
// {
// 	m_aOverrideRequests[(int)ESkinPrio::RAINBOW].m_NetworkClipped = true;
// }

bool CSkinInfoManager::NeedsNetMessage7()
{
	// TODO: use cached reference to avoid duplicated memory copy
	CTeeInfo Info = TeeInfo();

	// TODO: checking 0.6 values for 0.7 update makes little sense
	//       but it kinda works because we translate it first

	if(str_comp(m_LastInfoSent7.m_aSkinName, Info.m_aSkinName))
		return true;
	if(m_LastInfoSent7.m_ColorBody != Info.m_ColorBody)
		return true;
	if(m_LastInfoSent7.m_ColorFeet != Info.m_ColorFeet)
		return true;
	if(m_LastInfoSent7.m_UseCustomColor != Info.m_UseCustomColor)
		return true;

	return false;
}

void CSkinInfoManager::OnSendNetMessage7()
{
	// TODO: use cached reference to avoid duplicated memory copy
	CTeeInfo Info = TeeInfo();

	// TODO: storing 0.6 values for 0.7 update makes little sense
	//       but it kinda works because we translate it first
	str_copy(m_LastInfoSent7.m_aSkinName, Info.m_aSkinName);
	m_LastInfoSent7.m_ColorBody = Info.m_ColorBody;
	m_LastInfoSent7.m_ColorFeet = Info.m_ColorFeet;
	m_LastInfoSent7.m_UseCustomColor = Info.m_UseCustomColor;
}

void CSkinInfoManager::SetUserChoice(CTeeInfo Info)
{
	m_TeeInfoUserChoice = Info;
}

void CSkinInfoManager::SetSkinName(ESkinPrio Priority, const char *pSkinName)
{
	m_aOverrideRequests[(int)Priority].m_SkinName = std::string(pSkinName);
}

void CSkinInfoManager::UnsetSkinName(ESkinPrio Priority)
{
	m_aOverrideRequests[(int)Priority].m_SkinName = std::nullopt;
}

void CSkinInfoManager::SetColorBody(ESkinPrio Priority, int Color)
{
	m_aOverrideRequests[(int)Priority].m_ColorBody = Color;
}

void CSkinInfoManager::UnsetColorBody(ESkinPrio Priority)
{
	m_aOverrideRequests[(int)Priority].m_ColorBody = std::nullopt;
}

void CSkinInfoManager::SetColorFeet(ESkinPrio Priority, int Color)
{
	m_aOverrideRequests[(int)Priority].m_ColorFeet = Color;
}

void CSkinInfoManager::UnsetColorFeet(ESkinPrio Priority)
{
	m_aOverrideRequests[(int)Priority].m_ColorFeet = std::nullopt;
}

void CSkinInfoManager::SetUseCustomColor(ESkinPrio Priority, bool Value)
{
	m_aOverrideRequests[(int)Priority].m_UseCustomColor = Value;
}

void CSkinInfoManager::UnsetUseCustomColor(ESkinPrio Priority)
{
	m_aOverrideRequests[(int)Priority].m_UseCustomColor = std::nullopt;
}

void CSkinInfoManager::UnsetAll(ESkinPrio Priority)
{
	UnsetSkinName(Priority);
	UnsetColorBody(Priority);
	UnsetColorFeet(Priority);
	UnsetUseCustomColor(Priority);
}

void CSkinInfoManager::SkinName(char *pSkinNameOut, int SizeOfSkinNameOut)
{
	for(int i = (int)ESkinPrio::NUM_SKINPRIOS - 1; i > 0; i--)
	{
		if(!m_aOverrideRequests[i].m_SkinName.has_value())
			continue;
		str_copy(pSkinNameOut, m_aOverrideRequests[i].m_SkinName.value().c_str(), SizeOfSkinNameOut);
		return;
	}
	str_copy(pSkinNameOut, m_TeeInfoUserChoice.m_aSkinName, SizeOfSkinNameOut);
}

int CSkinInfoManager::ColorBody()
{
	for(int i = (int)ESkinPrio::NUM_SKINPRIOS - 1; i > 0; i--)
	{
		if(!m_aOverrideRequests[i].m_ColorBody.has_value())
			continue;
		return m_aOverrideRequests[i].m_ColorBody.value();
	}
	return m_TeeInfoUserChoice.m_ColorBody;
}

int CSkinInfoManager::ColorFeet()
{
	for(int i = (int)ESkinPrio::NUM_SKINPRIOS - 1; i > 0; i--)
	{
		if(!m_aOverrideRequests[i].m_ColorFeet.has_value())
			continue;
		return m_aOverrideRequests[i].m_ColorFeet.value();
	}
	return m_TeeInfoUserChoice.m_ColorFeet;
}

bool CSkinInfoManager::UseCustomColor()
{
	for(int i = (int)ESkinPrio::NUM_SKINPRIOS - 1; i > 0; i--)
	{
		if(!m_aOverrideRequests[i].m_UseCustomColor.has_value())
			continue;
		return m_aOverrideRequests[i].m_UseCustomColor.value();
	}
	return m_TeeInfoUserChoice.m_UseCustomColor;
}

CTeeInfo CSkinInfoManager::TeeInfo()
{
	// TODO: cache this
	CTeeInfo Info = m_TeeInfoUserChoice;
	SkinName(Info.m_aSkinName, sizeof(Info.m_aSkinName));
	Info.m_ColorBody = ColorBody();
	Info.m_ColorFeet = ColorFeet();
	Info.m_UseCustomColor = UseCustomColor();

	// TODO: do we need to support setting 0.7 specific skins?
	Info.ToSixup();

	return Info;
}
