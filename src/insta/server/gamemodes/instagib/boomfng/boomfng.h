#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_BOOMFNG_BOOMFNG_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_BOOMFNG_BOOMFNG_H

#include "../team_fng.h"

#define SQL_COLUMN_FILE <insta/server/gamemodes/instagib/boomfng/sql_columns.h>
#define SQL_COLUMN_CLASS CBoomfngColumns
#include <insta/server/column_template.h>

class CGameControllerBoomfng : public CGameControllerTeamFng
{
public:
	CGameControllerBoomfng(class CGameContext *pGameServer);
	~CGameControllerBoomfng() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
};
#endif
