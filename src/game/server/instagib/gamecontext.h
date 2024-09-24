#ifndef GAME_SERVER_INSTAGIB_GAMECONTEXT_H
#define GAME_SERVER_INSTAGIB_GAMECONTEXT_H

#include <game/server/gamecontext.h>

class CInstaGameContext : public CGameContext
{
public:
	void OnInit(const void *pPersistentData) override;

	void OnInitInstagib();

	void SendMotd(int ClientId) const;
};

#endif

