#ifndef INSTA_SERVER_GAMEMODES_DDRACE_BLOCK_TBLOCK_H
#define INSTA_SERVER_GAMEMODES_DDRACE_BLOCK_TBLOCK_H

#include <insta/server/gamemodes/ddrace/block/block.h>

class CGameControllerTBlock : public CGameControllerBlock
{
public:
	CGameControllerTBlock(class CGameContext *pGameServer);
	~CGameControllerTBlock() override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int WeaponId) override;
};
#endif
