#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_FNG_FNG_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_FNG_FNG_H

#include "../team_fng.h"

#define SQL_COLUMN_FILE <insta/server/gamemodes/instagib/fng/sql_columns.h>
#define SQL_COLUMN_CLASS CFngColumns
#include <insta/server/column_template.h>

class CGameControllerFng : public CGameControllerTeamFng
{
public:
	CGameControllerFng(class CGameContext *pGameServer);
	~CGameControllerFng() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
};
#endif
