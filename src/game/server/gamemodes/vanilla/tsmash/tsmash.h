#ifndef GAME_SERVER_GAMEMODES_VANILLA_TSMASH_TSMASH_H
#define GAME_SERVER_GAMEMODES_VANILLA_TSMASH_TSMASH_H

#include "../../vanilla/base_vanilla.h"

#include <memory>
#include <unordered_map>

class CGameControllerTsmash : public CGameControllerVanilla
{
private:
	void SetTeeColor(CPlayer *pPlayer);
	int m_aLastHealth[MAX_CLIENTS];
	std::unordered_map<int, std::unique_ptr<CEntity>> m_SuperSmash;

public:
	CGameControllerTsmash(class CGameContext *pGameServer, bool Teams);
	~CGameControllerTsmash() override;

	void OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
	void OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter) override;
	bool DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter) override;
	bool OnSelfkill(int ClientId) override;
};

#endif
