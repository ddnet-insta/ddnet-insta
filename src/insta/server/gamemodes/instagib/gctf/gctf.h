#ifndef INSTA_SERVER_GAMEMODES_INSTAGIB_GCTF_GCTF_H
#define INSTA_SERVER_GAMEMODES_INSTAGIB_GCTF_GCTF_H

#include <insta/server/gamemodes/instagib/ctf.h>

#define SQL_COLUMN_FILE <insta/server/gamemodes/instagib/gctf/sql_columns.h>
#define SQL_COLUMN_CLASS CGCTFColumns
#include <insta/server/column_template.h>

class CGameControllerGCTF : public CGameControllerInstaBaseCTF
{
public:
	CGameControllerGCTF(class CGameContext *pGameServer);
	~CGameControllerGCTF() override;

	void OnCharacterSpawn(class CCharacter *pChr) override;
	void Tick() override;
};
#endif
