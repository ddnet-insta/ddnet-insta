#ifndef INSTA_SERVER_GAMEMODES_VANILLA_DM_DM_H
#define INSTA_SERVER_GAMEMODES_VANILLA_DM_DM_H

#include "../base_vanilla.h"

#define SQL_COLUMN_FILE <insta/server/gamemodes/vanilla/dm/sql_columns.h>
#define SQL_COLUMN_CLASS CDmColumns
#include <insta/server/column_template.h>

class CGameControllerDM : public CGameControllerVanilla
{
public:
	CGameControllerDM(class CGameContext *pGameServer);
	~CGameControllerDM() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
};
#endif
