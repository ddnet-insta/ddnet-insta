#ifndef GAME_SERVER_GAMEMODES_VANILLA_TSMASH_TSMASH_H
#define GAME_SERVER_GAMEMODES_VANILLA_TSMASH_TSMASH_H

#include "../../vanilla/base_vanilla.h"

#include <memory>

class CGameControllerTsmash : public CGameControllerVanilla
{
private:
	void SetTeeColor(CPlayer *pPlayer);
	int m_aLastHealth[MAX_CLIENTS];
	std::unique_ptr<CEntity> m_aSuperSmash[MAX_CLIENTS];

public:
	CGameControllerTsmash(class CGameContext *pGameServer, bool Teams);
	~CGameControllerTsmash() override;

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void GiveSuperSmash(int ClientId, int Amount); // Amount can be negative
	void OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
	void OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	bool DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter) override;
	bool CanSelfkill(class CPlayer *pPlayer, char *pErrorReason, int ErrorReasonSize) override;
};

#endif
