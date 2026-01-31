#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_ICTF_ICTF_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_ICTF_ICTF_H

#include <insta/server/gamemodes/instagib/ctf.h>

#define SQL_COLUMN_FILE <insta/server/gamemodes/instagib/ictf/sql_columns.h>
#define SQL_COLUMN_CLASS CICTFColumns
#include <insta/server/column_template.h>

class CGameControllerICTF : public CGameControllerInstaBaseCTF
{
public:
	CGameControllerICTF(class CGameContext *pGameServer);
	~CGameControllerICTF() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
};
#endif
