#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_IDM_IDM_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_IDM_IDM_H

#include <insta/server/gamemodes/instagib/base_instagib.h>

#define SQL_COLUMN_FILE <insta/server/gamemodes/instagib/idm/sql_columns.h>
#define SQL_COLUMN_CLASS CIdmColumns
#include <insta/server/column_template.h>

class CGameControllerIDM : public CGameControllerBaseInstagib
{
public:
	CGameControllerIDM(class CGameContext *pGameServer);
	~CGameControllerIDM() override;

	void OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
};
#endif
