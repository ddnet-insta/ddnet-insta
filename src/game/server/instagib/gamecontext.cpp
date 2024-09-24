
#include "gamecontext.h"
#include "base/system.h"

void CInstaGameContext::OnInit(const void *pPersistentData)
{
	CGameContext::OnInit(pPersistentData);

	OnInitInstagib(); // ddnet-insta
}

void CInstaGameContext::OnInitInstagib()
{
	UpdateVoteCheckboxes(); // ddnet-insta
	AlertOnSpecialInstagibConfigs(); // ddnet-insta
	ShowCurrentInstagibConfigsMotd(); // ddnet-insta

	m_pHttp = Kernel()->RequestInterface<IHttp>();
}


void CInstaGameContext::SendMotd(int ClientId) const
{
	dbg_msg("insta", "sending motd ....... ");
}
