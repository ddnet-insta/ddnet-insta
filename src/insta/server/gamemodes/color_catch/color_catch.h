#ifndef INSTA_SERVER_GAMEMODES_COLOR_CATCH_COLOR_CATCH_H
#define INSTA_SERVER_GAMEMODES_COLOR_CATCH_COLOR_CATCH_H

#include <insta/server/gamemodes/instagib/base_instagib.h>

// uncomment the lines below if you bring your own sql_columns.h file
// this is optional and only needed if your gamemode needs additional
// columns in the sql database.
// #define SQL_COLUMN_FILE <insta/server/gamemodes/color_catch/sql_columns.h>
// #define SQL_COLUMN_CLASS CColorCatchColumns
// #include <game/server/instagib/column_template.h>

class CGameControllerColorCatch : public CGameControllerBaseInstagib
{
public:
	CGameControllerColorCatch(CGameContext *pGameServer);
	~CGameControllerColorCatch() override;

	void OnInit(bool ServerStart) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterDeath(class CCharacter *pVictim, CPlayer *pKiller, int Weapon) override;
};
#endif
