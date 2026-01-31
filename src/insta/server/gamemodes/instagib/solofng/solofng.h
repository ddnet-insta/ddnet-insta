#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_SOLOFNG_SOLOFNG_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_SOLOFNG_SOLOFNG_H

#include "../base_fng.h"

#define SQL_COLUMN_FILE <insta/server/gamemodes/instagib/solofng/sql_columns.h>
#define SQL_COLUMN_CLASS CSolofngColumns
#include <insta/server/column_template.h>

class CGameControllerSolofng : public CGameControllerBaseFng
{
public:
	CGameControllerSolofng(class CGameContext *pGameServer);
	~CGameControllerSolofng() override;

	void Tick() override;
	void Snap(int SnappingClient) override;
	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	bool OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number) override;
};
#endif
