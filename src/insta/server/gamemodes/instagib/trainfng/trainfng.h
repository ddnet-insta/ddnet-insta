#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_TRAINFNG_TRAINFNG_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_TRAINFNG_TRAINFNG_H

#include <insta/server/gamemodes/instagib/solofng/solofng.h>

class CGameControllerTrainFng : public CGameControllerSolofng
{
public:
	CGameControllerTrainFng(class CGameContext *pGameServer);
	~CGameControllerTrainFng() override;

	void Tick() override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
	void OnPauseChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void OnSpecChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	bool CanSelfkill(class CPlayer *pPlayer, char *pErrorReason, int ErrorReasonSize) override;
	bool OnChatMessage(const CNetMsg_Cl_Say *pMsg, int Length, int &Team, CPlayer *pPlayer) override;

	bool IsFngGameType() const override { return true; }
	bool IsTrainFngGameType() const override { return true; }
};
#endif
