#include "bomb.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/instagib/skin_info_manager.h>

#include <random>

std::mt19937 CGameControllerBomb::M_S_RANDOM_ENGINE(std::random_device{}());

CGameControllerBomb::CGameControllerBomb(class CGameContext *pGameServer) :
	CGameControllerBasePvp(pGameServer)
{
	m_pGameType = "BOMB";
	m_GameFlags = 0;

	m_DefaultWeapon = WEAPON_HAMMER;

	m_pStatsTable = "bomb";
	m_pExtraColumns = new CBombColumns();
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerBomb::~CGameControllerBomb() = default;

void CGameControllerBomb::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta BOMB created by ByFox in 2025",
		"BOMB was originally created by Somerunce in 2009",
		"https://www.teeworlds.com/forum/viewtopic.php?id=3965",
		"https://git.ddstats.tw/furo/ddnet-bombtag",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerBomb::Tick()
{
	CGameControllerBasePvp::Tick();

	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		// In the source code it was done via GetAutoTeam, but it crashes...
		if(pPlayer->m_BombState == CPlayer::EBombState::NONE)
			pPlayer->m_BombState = pPlayer->GetTeam() == TEAM_SPECTATORS ? CPlayer::EBombState::SPECTATING : CPlayer::EBombState::ACTIVE;

		if(!m_RoundActive)
			continue;

		if(pPlayer->m_BombState == CPlayer::EBombState::ACTIVE && !m_Warmup && pPlayer->GetTeam() != TEAM_SPECTATORS)
			pPlayer->SetTeam(TEAM_SPECTATORS, true);
	}

	if(m_RoundActive)
	{
		for(const auto &pPlayer : GameServer()->m_apPlayers)
		{
			if(pPlayer)
				SetSkin(pPlayer);
		}
	}
	else
	{
		if(AmountOfPlayers(CPlayer::EBombState::ACTIVE) == 1)
		{
			switch(Server()->Tick() % (Server()->TickSpeed() * 3))
			{
			case 50:
				GameServer()->SendBroadcast("Waiting for players.", -1);
				break;
			case 100:
				GameServer()->SendBroadcast("Waiting for players..", -1);
				break;
			case 0:
				GameServer()->SendBroadcast("Waiting for players...", -1);
				break;
			}
		}
		if(AmountOfPlayers(CPlayer::EBombState::ACTIVE) + AmountOfPlayers(CPlayer::EBombState::ALIVE) > 1 && !m_Warmup)
		{
			GameServer()->SendBroadcast("Game started", -1);
			StartBombRound();
		}
	}
}

void CGameControllerBomb::OnReset()
{
	CGameControllerBasePvp::OnReset();

	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->m_BombState >= CPlayer::EBombState::ACTIVE)
		{
			pPlayer->m_BombState = CPlayer::EBombState::ACTIVE;
			pPlayer->m_IsBomb = false;
		}
	}
	m_RoundActive = false;
}

bool CGameControllerBomb::DoWincheckRound()
{
	if(!m_RoundActive || m_Warmup > 0)
		return false;

	if(Server()->ClientCount() <= 1)
	{
		EndRound();
		return true;
	}

	if(AmountOfBombs() == 0)
	{
		if(AmountOfPlayers(CPlayer::EBombState::ALIVE) >= 2)
		{
			int Alive = 0;
			for(auto *pPlayer : GameServer()->m_apPlayers)
			{
				if(!pPlayer)
					continue;

				if(pPlayer->m_BombState == CPlayer::EBombState::ALIVE && !pPlayer->m_IsBomb)
					Alive++;
			}
			MakeRandomBomb(std::ceil((Alive / (float)Config()->m_SvBombtagBombsPerPlayer) - (Config()->m_SvBombtagBombsPerPlayer == 1 ? 1 : 0)));
		}
		else
		{
			// A hack to avoid the end-of-round window.
			OnRoundEnd();
			return false;
		}
	}

	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->m_IsBomb)
		{
			if(pPlayer->m_ToBombTick % Server()->TickSpeed() == 0)
				UpdateTimer();
			if(pPlayer->m_ToBombTick <= 0)
				ExplodeBomb(pPlayer);
			pPlayer->m_ToBombTick--;
		}
	}

	return false;
}

void CGameControllerBomb::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBasePvp::OnCharacterSpawn(pChr);

	if(pChr->GetPlayer()->m_IsBomb)
		return;

	pChr->GiveWeapon(WEAPON_HAMMER, false, -1);
}

bool CGameControllerBomb::OnCharacterTakeDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter &Character)
{
	CPlayer *pPlayer = Character.GetPlayer();
	CPlayer *pKiller = GetPlayerOrNullptr(From);

	if(!pKiller)
		return false;

	if(pKiller->m_IsBomb && pKiller->m_ToBombTick > 0 && !pPlayer->m_IsBomb)
	{
		auto *pChr = pKiller->GetCharacter();
		if(!pChr)
			return false;

		GameServer()->SendBroadcast("", From);
		pKiller->m_IsBomb = false;

		// Remove all remaining projectiles from this player on the map
		GameServer()->m_World.RemoveEntitiesFromPlayer(From);
		MakeBomb(pPlayer->GetCid(), pKiller->m_ToBombTick);

		pChr->GiveWeapon(Config()->m_SvBombtagBombWeapon, true);
		pChr->GiveWeapon(WEAPON_HAMMER);
		pChr->SetWeapon(WEAPON_HAMMER);

		if(pPlayer->m_ToBombTick < Config()->m_SvBombtagMinSecondsToExplosion * Server()->TickSpeed() && Config()->m_SvBombtagMinSecondsToExplosion)
			pPlayer->m_ToBombTick = Config()->m_SvBombtagMinSecondsToExplosion * Server()->TickSpeed();

		Character.GiveWeapon(Config()->m_SvBombtagBombWeapon);
		Character.SetWeapon(Config()->m_SvBombtagBombWeapon);
		return false;
	}

	if(Weapon != WEAPON_HAMMER)
		return false;

	if(pPlayer->m_IsBomb)
	{
		pPlayer->m_ToBombTick -= Config()->m_SvBombtagBombDamage * Server()->TickSpeed();
		UpdateTimer();

		// Increase stats if they killed the player
		if(pPlayer->m_ToBombTick <= 0)
			pKiller->m_Stats.m_Kills++;
		return false;
	}

	return false;
}

// From CGameControllerInstagib::OnEntity
bool CGameControllerBomb::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	// ddnet-insta
	// do not spawn pickups
	if(Index == ENTITY_ARMOR_1 || Index == ENTITY_HEALTH_1 || Index == ENTITY_WEAPON_SHOTGUN || Index == ENTITY_WEAPON_GRENADE || Index == ENTITY_WEAPON_LASER || Index == ENTITY_POWERUP_NINJA)
		return false;

	return CGameControllerBasePvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}

bool CGameControllerBomb::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[NotThisId];
	if(!pPlayer)
		return true;

	if(!m_RoundActive && Team != TEAM_SPECTATORS)
	{
		if(pErrorReason)
			str_copy(pErrorReason, "", ErrorReasonSize);
		pPlayer->m_BombState = CPlayer::EBombState::ACTIVE;
		return true;
	}

	if(Team == TEAM_SPECTATORS)
	{
		if(pErrorReason)
			str_copy(pErrorReason, "You are a spectator now\nYou won't join when a new round begins", ErrorReasonSize);
		pPlayer->m_BombState = CPlayer::EBombState::SPECTATING;
		pPlayer->m_IsBomb = false;
		return true;
	}

	if(pErrorReason)
		str_copy(pErrorReason, "You will join the game when the round is over", ErrorReasonSize);
	pPlayer->m_BombState = CPlayer::EBombState::ACTIVE;
	return false;
}

void CGameControllerBomb::OnRoundEnd()
{
	bool WinnerAnnounced = false;
	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->m_BombState != CPlayer::EBombState::ALIVE)
			continue;

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "'%s' won the round!", Server()->ClientName(pPlayer->GetCid()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
		WinnerAnnounced = true;

		pPlayer->m_Stats.m_Wins++;
		pPlayer->m_Stats.m_Losses--;
		pPlayer->AddScore(1);
		break;
	}

	if(!WinnerAnnounced)
		GameServer()->SendChat(-1, TEAM_ALL, "Noone won the round!");

	DoWarmup(3);
	CGameControllerBasePvp::OnRoundEnd();
}

void CGameControllerBomb::OnShowStatsAll(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	CGameControllerBasePvp::OnShowStatsAll(pStats, pRequestingPlayer, pRequestedName);

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "~ Collateral Kills: %d", pStats->m_CollateralKills);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerBomb::OnShowRoundStats(const CSqlStatsPlayer *pStats, class CPlayer *pRequestingPlayer, const char *pRequestedName)
{
	CGameControllerBasePvp::OnShowRoundStats(pStats, pRequestingPlayer, pRequestedName);

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "~ Collateral Kills: %d", pStats->m_CollateralKills);
	GameServer()->SendChatTarget(pRequestingPlayer->GetCid(), aBuf);
}

void CGameControllerBomb::SetSkin(CPlayer *pPlayer)
{
	if(pPlayer->m_IsBomb)
	{
		// TODO: dont copy this string on tick but only when the bomb changes
		pPlayer->m_SkinInfoManager.SetSkinName(ESkinPrio::HIGH, "bomb");
		pPlayer->m_SkinInfoManager.SetUseCustomColor(ESkinPrio::HIGH, false);

		if(pPlayer->m_ToBombTick <= 3 * Server()->TickSpeed())
		{
			ColorRGBA Color = ColorRGBA(255 - pPlayer->m_ToBombTick, 0, 0);
			pPlayer->m_SkinInfoManager.SetColorBody(ESkinPrio::HIGH, color_cast<ColorHSLA>(Color).PackAlphaLast(false));
			pPlayer->m_SkinInfoManager.SetColorFeet(ESkinPrio::HIGH, 16777215); // white
			pPlayer->m_SkinInfoManager.SetUseCustomColor(ESkinPrio::HIGH, true);
		}
		return;
	}

	// Ignores manual installation of the bomb skin
	if(str_comp_nocase(pPlayer->m_SkinInfoManager.UserChoice().m_aSkinName, "bomb") == 0)
	{
		pPlayer->m_SkinInfoManager.SetSkinName(ESkinPrio::HIGH, "default");
		return;
	}
	pPlayer->m_SkinInfoManager.UnsetAll(ESkinPrio::HIGH);
}

void CGameControllerBomb::EliminatePlayer(CPlayer *pPlayer, bool Collateral)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' eliminated%s!", Server()->ClientName(pPlayer->GetCid()), Collateral ? " by collateral damage" : "");
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);

	pPlayer->m_IsBomb = false;
	pPlayer->m_BombState = CPlayer::EBombState::ACTIVE;
}

void CGameControllerBomb::ExplodeBomb(CPlayer *pPlayer)
{
	GameServer()->CreateExplosion(pPlayer->m_ViewPos, pPlayer->GetCid(), WEAPON_GAME, true, 0);
	GameServer()->CreateSound(pPlayer->m_ViewPos, SOUND_GRENADE_EXPLODE);
	pPlayer->m_BombState = CPlayer::EBombState::ACTIVE;

	// Collateral damage
	for(auto *pTempPlayer : GameServer()->m_apPlayers)
	{
		if(!Config()->m_SvBombtagCollateralDamage)
			break;

		if(!pTempPlayer)
			continue;

		CCharacter *pChr = pTempPlayer->GetCharacter();
		if(!pChr)
			continue;

		if(pPlayer->GetCid() == pTempPlayer->GetCid())
			continue;

		if(distance(pTempPlayer->m_ViewPos, pPlayer->m_ViewPos) <= 96)
		{
			pPlayer->KillCharacter();
			EliminatePlayer(pTempPlayer, true);

			pPlayer->m_Stats.m_CollateralKills++;
		}
	}

	// We remove the projectiles of a player who has already exploded.
	GameServer()->m_World.RemoveEntitiesFromPlayer(pPlayer->GetCid());
	pPlayer->KillCharacter();
	EliminatePlayer(pPlayer);
}

void CGameControllerBomb::UpdateTimer()
{
	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer || !pPlayer->m_IsBomb)
			continue;

		auto *pChr = pPlayer->GetCharacter();
		if(!pChr)
			continue;

		GameServer()->CreateDamageInd(pPlayer->m_ViewPos, 0, pPlayer->m_ToBombTick / Server()->TickSpeed());
		SetArmorProgress(pChr, pPlayer->m_ToBombTick / Server()->TickSpeed());
	}
}

void CGameControllerBomb::StartBombRound()
{
	int Players = 0;
	m_RoundActive = true;

	for(auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->m_BombState != CPlayer::EBombState::ACTIVE)
			continue;

		pPlayer->SetTeam(TEAM_FLOCK, true);
		pPlayer->m_BombState = CPlayer::EBombState::ALIVE;
		Players++;

		// Instant appearance after the start of the game
		pPlayer->Respawn();
	}
	MakeRandomBomb(std::ceil((Players / static_cast<float>(Config()->m_SvBombtagBombsPerPlayer)) - (Config()->m_SvBombtagBombsPerPlayer == 1 ? 1 : 0)));
}

void CGameControllerBomb::MakeRandomBomb(int Count)
{
	int Playing[MAX_CLIENTS];
	int Players = 0;

	for(const auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->m_BombState == CPlayer::EBombState::ALIVE)
			Playing[Players++] = pPlayer->GetCid();
	}

	std::shuffle(Playing, Playing + Players, M_S_RANDOM_ENGINE);

	for(int i = 0; i < Count; i++)
	{
		MakeBomb(Playing[i], Config()->m_SvBombtagSecondsToExplosion * Server()->TickSpeed());
	}
}

void CGameControllerBomb::MakeBomb(int ClientId, int Ticks)
{
	CPlayer *pPlayer = GameServer()->m_apPlayers[ClientId];
	if(!pPlayer)
		return;

	CCharacter *pChr = pPlayer->GetCharacter();
	if(!pChr)
		return;

	// clear previous broadcast
	GameServer()->SendBroadcast("", ClientId);

	pPlayer->m_IsBomb = true;
	pPlayer->m_ToBombTick = Ticks;
	if(Config()->m_SvBombtagBombWeapon != WEAPON_HAMMER)
		pChr->GiveWeapon(WEAPON_HAMMER, true);

	pChr->GiveWeapon(Config()->m_SvBombtagBombWeapon);
	pChr->SetWeapon(Config()->m_SvBombtagBombWeapon);

	GameServer()->SendBroadcast("You are the new bomb!\nHit another player before the time runs out!", ClientId);
}

int CGameControllerBomb::AmountOfPlayers(CPlayer::EBombState State) const
{
	int Amount = 0;
	for(const auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(pPlayer && pPlayer->m_BombState == State)
			Amount++;
	}
	return Amount;
}

int CGameControllerBomb::AmountOfBombs() const
{
	int Amount = 0;
	for(const auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(pPlayer && pPlayer->m_IsBomb)
			Amount++;
	}
	return Amount;
}

REGISTER_GAMEMODE(bomb, CGameControllerBomb(pGameServer));
