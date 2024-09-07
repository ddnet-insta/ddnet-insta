#ifndef GAME_SERVER_GAMEMODES_INSTAGIB_CTF_H
#define GAME_SERVER_GAMEMODES_INSTAGIB_CTF_H

#include <engine/shared/protocol.h>

#include <game/server/gamemodes/instagib/base_instagib.h>

class CGameControllerInstaBaseCTF : public CGameControllerInstagib
{
	/*
		m_aapFlags

		One flag pair per ddrace team.
		So all 64 or soon 128 teams can have their own red and blue flag.
	*/
	// class CFlag *m_aapFlags[MAX_CLIENTS][2];
	class CFlag *m_aapFlags[MAX_CLIENTS][2];
	bool DoWincheckRound() override;

public:
	CGameControllerInstaBaseCTF(class CGameContext *pGameServer);
	~CGameControllerInstaBaseCTF() override;

	void Tick() override;
	void Snap(int SnappingClient) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnFlagReturn(class CFlag *pFlag) override;
	void OnFlagGrab(class CFlag *pFlag) override;
	void OnFlagCapture(class CFlag *pFlag, float Time) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;

	void FlagTick();
};
#endif
