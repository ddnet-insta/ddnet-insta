#include "bomb.h"

#include <base/dbg.h>
#include <base/log.h>

#include <engine/server.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

#include <insta/server/gamemodes/base_pvp/base_pvp.h>
#include <insta/server/skin_info_manager.h>

#include <random>

std::mt19937 CGameControllerBomb::M_S_RANDOM_ENGINE(std::random_device{}());

CGameControllerBomb::CGameControllerBomb(class CGameContext *pGameServer) :
	CGameControllerBasePvp(pGameServer)
{
	m_pGameType = "BOMB";
	m_WinType = WIN_BY_SURVIVAL;
	m_GameFlags = 0;
	m_DefaultWeapon = WEAPON_HAMMER;
	m_pDeadSpecController = new CDeadSpecController(this, pGameServer);
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

		pPlayer->ResetLastToucherAfterSeconds(3);
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
		if(NumActivePlayers() > 1 && !m_Warmup)
		{
			GameServer()->SendBroadcast("Game started", -1);
			StartBombRound();
		}
		else if(!m_Warmup)
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
	}
}

void CGameControllerBomb::OnReset()
{
	CGameControllerBasePvp::OnReset();

	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		pPlayer->m_IsBomb = false;
	}
	m_RoundActive = false;
}

int CGameControllerBomb::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	EliminatePlayer(pVictim->GetPlayer(), Weapon);
	return CGameControllerBasePvp::OnCharacterDeath(pVictim, pKiller, Weapon);
}

void CGameControllerBomb::OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg)
{
	CPlayer *pKiller = GetPlayerOrNullptr(Killer);
	// if the hammer directly caused a bomb explosion
	if(pKiller && pKiller != pVictim->GetPlayer())
	{
		CGameControllerBasePvp::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
		return;
	}
	std::optional<CLastToucher> &LastToucher = pVictim->GetPlayer()->m_LastToucher;

	// died alone without any killer
	if(!LastToucher.has_value())
	{
		// do not count the kill
		CGameControllerBasePvp::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
		return;
	}

	int LastToucherId = LastToucher.value().m_ClientId;
	pKiller = GetPlayerOrNullptr(LastToucherId);

	if(pKiller && pKiller != pVictim->GetPlayer())
	{
		// count the kill
		CGameControllerBasePvp::OnCharacterDeathImpl(
			pVictim,
			pKiller->GetCid(),
			LastToucher.value().m_Weapon,
			SendKillMsg);
		return;
	}

	// do not count the kill
	CGameControllerBasePvp::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
}

bool CGameControllerBomb::DoWincheckRound()
{
	if(!m_RoundActive || m_Warmup > 0)
		return false;

	// also allow winning by reaching scorelimit
	// if sv_scorelimit is set
	// https://github.com/ddnet-insta/ddnet-insta/issues/558
	if(IGameController::DoWincheckRound())
		return true;

	if(Server()->ClientCount() <= 1)
	{
		EndRound();
		JoinAllPlayers();
		return true;
	}

	int AlivePlayers = NumNonDeadActivePlayers();

	if(AmountOfBombs() == 0 || AlivePlayers <= 1)
	{
		if(AlivePlayers >= 2)
		{
			int Alive = 0;
			for(auto *pPlayer : GameServer()->m_apPlayers)
			{
				if(!pPlayer)
					continue;

				if(!pPlayer->m_IsDead && !pPlayer->m_IsBomb)
					Alive++;
			}
			// TODO: why is the wincheck creating bombs?
			MakeRandomBomb(std::ceil((Alive / (float)Config()->m_SvBombtagBombsPerPlayer) - (Config()->m_SvBombtagBombsPerPlayer == 1 ? 1 : 0)));
		}
		else
		{
			// A hack to avoid the end-of-round window.
			OnRoundEnd();
			JoinAllPlayers();
			return false;
		}
	}

	// TODO: why is this logic in the wincheck?
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

void CGameControllerBomb::OnPlayerConnect(CPlayer *pPlayer)
{
	CGameControllerBasePvp::OnPlayerConnect(pPlayer);

	if(m_RoundActive)
	{
		// helps with "reload" command see https://github.com/ddnet-insta/ddnet-insta/issues/555
		// also its nice if players that are just a little bit late can still join
		// this can't really be abused with reconnects because the time window is so tight
		int RoundSeconds = (Server()->Tick() - m_RoundStartTick) / Server()->TickSpeed();
		if(RoundSeconds > 2)
		{
			m_pDeadSpecController->KillPlayer(pPlayer, -1);
			GameServer()->SendChatTarget(pPlayer->GetCid(), "You have to wait for the round to end before you can join");
		}
	}
}

void CGameControllerBomb::OnAppliedDamage(int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	if(!pCharacter)
		return;

	CGameControllerBasePvp::OnAppliedDamage(Dmg, From, Weapon, pCharacter);

	CPlayer *pPlayer = pCharacter->GetPlayer();
	CPlayer *pKiller = GetPlayerOrNullptr(From);

	if(!pKiller)
		return;

	if(pKiller->m_IsBomb && pKiller->m_ToBombTick > 0 && !pPlayer->m_IsBomb)
	{
		auto *pChr = pKiller->GetCharacter();
		if(!pChr)
			return;

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

		pCharacter->GiveWeapon(Config()->m_SvBombtagBombWeapon);
		pCharacter->SetWeapon(Config()->m_SvBombtagBombWeapon);
		return;
	}

	if(Weapon != WEAPON_HAMMER)
		return;

	if(pPlayer->m_IsBomb)
	{
		pPlayer->m_ToBombTick -= Config()->m_SvBombtagBombDamage * Server()->TickSpeed();
		UpdateTimer();

		// Increase stats if they killed the player
		if(pPlayer->m_ToBombTick <= 0)
		{
			ExplodeBomb(pPlayer, pKiller);
		}
	}
}

bool CGameControllerBomb::OnEntity(int Index, int x, int y, int Layer, int Flags, bool Initial, int Number)
{
	if(IsPickupEntity(Index))
		return false;

	return CGameControllerBasePvp::OnEntity(Index, x, y, Layer, Flags, Initial, Number);
}

bool CGameControllerBomb::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
{
	if(!CGameControllerBasePvp::CanJoinTeam(Team, NotThisId, pErrorReason, ErrorReasonSize))
		return false;

	CPlayer *pPlayer = GameServer()->m_apPlayers[NotThisId];
	if(!pPlayer)
		return false;

	if(pPlayer->m_IsDead && Team != TEAM_SPECTATORS)
	{
		// TODO: this error message is overwritten by the
		//       "You will join the spectators once the round ends" message
		if(pErrorReason)
			str_copy(pErrorReason, "Wait until round end", ErrorReasonSize);
		return false;
	}
	return true;
}

void CGameControllerBomb::OnRoundEnd()
{
	bool WinnerAnnounced = false;
	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		if(!IsWinner(pPlayer, nullptr, 0))
			continue;

		char aBuf[128];
		str_format(
			aBuf,
			sizeof(aBuf),
			"'%s' won the round!%s",
			Server()->ClientName(pPlayer->GetCid()),
			pPlayer->m_IsBomb ? " (as bomb! +1 extra win point)" : "");
		GameServer()->SendChat(-1, TEAM_ALL, aBuf);
		WinnerAnnounced = true;
		break;
	}

	if(!WinnerAnnounced)
		GameServer()->SendChat(-1, TEAM_ALL, "Noone won the round!");

	m_RoundActive = false;
	DoWarmup(3);
	CGameControllerBasePvp::OnRoundEnd();
}

void CGameControllerBomb::YouWillJoinSpecMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the spectators once the round ends", MsgLen);
}

void CGameControllerBomb::YouWillJoinGameMessage(CPlayer *pPlayer, char *pMsg, size_t MsgLen)
{
	str_copy(pMsg, "You will join the game once the round ends", MsgLen);
}

bool CGameControllerBomb::IsWinner(const CPlayer *pPlayer, char *pMessage, int SizeOfMessage)
{
	if(pMessage && SizeOfMessage)
		pMessage[0] = '\0';

	// you can only win as last alive player
	// used for disconnect IsWinner check
	if(NumNonDeadActivePlayers() > 1)
		return false;
	if(pPlayer->GetTeam() == TEAM_SPECTATORS)
		return false;
	if(pPlayer->m_IsDead)
		return false;
	if(!m_RoundActive)
		return false;

	// if(pMessage)
	// 	str_copy(pMessage, "+1 win was saved on your name (see /rank_wins).", SizeOfMessage);

	return true;
}

bool CGameControllerBomb::IsLoser(const CPlayer *pPlayer)
{
	// you can only win running games
	// so you can also only lose running games
	if(m_RoundActive)
		return false;

	// rage quit as dead player is counted as a loss
	// qutting mid game while being alive is not
	return pPlayer->m_IsDead;
}

int CGameControllerBomb::WinPointsForWin(const CPlayer *pPlayer)
{
	int Kills = pPlayer->m_RoundStats.m_Kills;
	int Points = 0;
	// yes it is possible to make 0 win points
	// even if winning
	if(Kills < 1) // 0
		Points = 0;
	else if(Kills <= 6) // 1-6
		Points = 1;
	else if(Kills <= 8) // 7-8
		Points = 2;
	else if(Kills == 9) // 9
		Points = 3;
	else if(Kills == 10) // 10
		Points = 5;
	else if(Kills == 11) // 11
		Points = 7;
	else if(Kills == 12) // 12
		Points = 9;
	else if(Kills == 13) // 13
		Points = 11;
	else if(Kills == 14) // 14
		Points = 12;
	else if(Kills == 15) // 15
		Points = 14;
	else // 16+
		Points = 16;

	// https://github.com/ddnet-insta/ddnet-insta/issues/551
	if(pPlayer->m_IsBomb)
		Points++;

	log_info(
		"bomb",
		"player '%s' earned %d win_points for winning with %d kills%s",
		Server()->ClientName(pPlayer->GetCid()),
		Points,
		Kills,
		pPlayer->m_IsBomb ? " (win as bomb bonus)" : "");
	return Points;
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

void CGameControllerBomb::EliminatePlayer(CPlayer *pPlayer, int Weapon)
{
	// https://github.com/ddnet-insta/ddnet-insta/issues/570
	if(!m_RoundActive)
		return;
	if(pPlayer->m_IsDead)
		return;

	bool Collateral = Weapon == WEAPON_BOMB;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "'%s' eliminated%s!", Server()->ClientName(pPlayer->GetCid()), Collateral ? " by collateral damage" : "");
	GameServer()->SendChat(-1, TEAM_ALL, aBuf);

	pPlayer->m_IsBomb = false;
	m_pDeadSpecController->KillPlayer(pPlayer, -1);
}

void CGameControllerBomb::ExplodeBomb(CPlayer *pPlayer, CPlayer *pKiller)
{
	dbg_assert(pPlayer->GetCharacter(), "dead player with cid=%d exploded", pPlayer->GetCid());

	if(!pPlayer->m_IsBomb)
		return;

	// Collateral damage
	if(!pKiller && Config()->m_SvBombtagCollateralDamage)
	{
		for(auto *pTempPlayer : GameServer()->m_apPlayers)
		{
			if(!pTempPlayer || pTempPlayer->GetTeam() == TEAM_SPECTATORS)
				continue;

			if(!pTempPlayer->GetCharacter() || !pTempPlayer->GetCharacter()->IsAlive())
				continue;

			if(pPlayer->GetCid() == pTempPlayer->GetCid())
				continue;

			if(distance(pTempPlayer->m_ViewPos, pPlayer->m_ViewPos) <= 96)
			{
				pTempPlayer->GetCharacter()->Die(pPlayer->GetCid(), WEAPON_BOMB);
				pPlayer->m_Stats.m_CollateralKills++;
			}
		}
	}

	// We remove the projectiles of a player who has already exploded.
	GameServer()->m_World.RemoveEntitiesFromPlayer(pPlayer->GetCid());
	pPlayer->GetCharacter()->Die(
		pKiller ? pKiller->GetCid() : pPlayer->GetCid(),
		pKiller ? (int)WEAPON_HAMMER : (int)WEAPON_GAME);
	GameServer()->CreateExplosion(pPlayer->m_ViewPos, pPlayer->GetCid(), WEAPON_GAME, true, 0);
	GameServer()->CreateSound(pPlayer->m_ViewPos, SOUND_GRENADE_EXPLODE);
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
		if(pPlayer->GetTeam() == TEAM_SPECTATORS)
			continue;

		Players++;

		// Instant appearance after the start of the game
		pPlayer->Respawn();
	}
	MakeRandomBomb(std::ceil((Players / static_cast<float>(Config()->m_SvBombtagBombsPerPlayer)) - (Config()->m_SvBombtagBombsPerPlayer == 1 ? 1 : 0)));
}

void CGameControllerBomb::MakeRandomBomb(int Count)
{
	int aPlayingIds[MAX_CLIENTS] = {-1};
	int Players = 0;

	for(const auto &pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(!pPlayer->m_IsDead)
			aPlayingIds[Players++] = pPlayer->GetCid();
	}

	std::shuffle(aPlayingIds, aPlayingIds + Players, M_S_RANDOM_ENGINE);
	if(Count > Players)
		Count = Players;

	for(int i = 0; i < Count; i++)
	{
		MakeBomb(aPlayingIds[i], Config()->m_SvBombtagSecondsToExplosion * Server()->TickSpeed());
	}
}

void CGameControllerBomb::MakeBomb(int ClientId, int Ticks)
{
	CPlayer *pPlayer = GetPlayerOrNullptr(ClientId);
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

void CGameControllerBomb::JoinAllPlayers()
{
	m_pDeadSpecController->RespawnAllPlayers();

	// TODO: zCatch clears victims here
	//       we clear m_IsBomb in OnReset
	//       use the same approach for both gametypes
}

REGISTER_GAMEMODE(bomb, CGameControllerBomb(pGameServer));
