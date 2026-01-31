#ifndef GAME_SERVER_GAMEMODES_DDRACE_DDRACE_DDRACE_H
#define GAME_SERVER_GAMEMODES_DDRACE_DDRACE_DDRACE_H

#include <insta/server/gamemodes/insta_core/insta_core.h>

class CPlayer;

// This is a ddnet-insta mode not a official ddnet mode.
// It is regular ddnet plus ddnet-insta extensions.
//
// To play this gamemode use `sv_gametype ddrace` in your config
// For as pure ddnet as possible use `sv_gametype ddnet`
class CGameControllerDDRace : public CGameControllerInstaCore
{
public:
	CGameControllerDDRace(CGameContext *pGameServer);
	~CGameControllerDDRace() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	bool OnTeamChatCmd(IConsole::IResult *pResult) override;
};
#endif
