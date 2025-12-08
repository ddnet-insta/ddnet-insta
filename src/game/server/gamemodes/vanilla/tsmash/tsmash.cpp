#include "tsmash.h"

#include <base/color.h>

#include <engine/shared/config.h>

#include <generated/protocol.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamemodes/base_pvp/base_pvp.h>
#include <game/server/instagib/entities/ddnet_pvp/vanilla_pickup.h>

class CParticleCircle : public CEntity
{
private:
	int m_aParticles[5];
	static constexpr float RADIUS = 30.0f;
	const int m_ClientId;

public:
	CParticleCircle(CGameWorld *pGameWorld, int ClientId, int Amount) :
		CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, vec2(0.0f, 0.0f), RADIUS), m_ClientId(ClientId)
	{
		for(int &Particle : m_aParticles)
			Particle = Server()->SnapNewId();
		m_Number = Amount;
		GameWorld()->InsertEntity(this);
	}
	~CParticleCircle() override
	{
		for(const int &Particle : m_aParticles)
			Server()->SnapFreeId(Particle);
	}
	void Tick() override
	{
		auto *pPlayer = this->GameServer()->m_apPlayers[m_ClientId];
		if(!pPlayer)
			return;
		auto *pChar = pPlayer->GetCharacter();
		if(!pChar)
			return;
		m_Pos = pChar->GetPos();
	}
	void Snap(int SnappingClient) override
	{
		const float Tick = (float)Server()->Tick() / 8.0f * (float)m_Number;
		for(int i = 0; i < (int)std::size(m_aParticles); ++i)
		{
			const int &Particle = m_aParticles[i];

			float Angle = (float)i / (float)std::size(m_aParticles) * (2.0f * pi) + Tick;
			vec2 Pos = m_Pos + direction(Angle) * RADIUS;

			CNetObj_Projectile *pObj = Server()->SnapNewItem<CNetObj_Projectile>(Particle);
			if(!pObj)
				return;

			pObj->m_X = Pos.x;
			pObj->m_Y = Pos.y;
			pObj->m_VelX = 0;
			pObj->m_VelY = 0;
			pObj->m_Type = WEAPON_HAMMER;
			pObj->m_StartTick = Server()->Tick();
		}
	}
};

void CGameControllerTsmash::OnCreditsChatCmd(IConsole::IResult *pResult, void *pUserData)
{
	static constexpr const char *CREDITS[] = {
		"ddnet-insta tsmash created by SollyBunny in 2025",
		"teesmash was originally created by Ryozuki & timakro in 2016",
		"https://github.com/edg-l/teesmash",
		"For more information see /credits_insta",
	};
	for(const char *pLine : CREDITS)
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "chatresp", pLine);
}

void CGameControllerTsmash::GiveSuperSmash(int ClientId, int Amount)
{
	if(Amount == 0)
		return;
	if(Amount > 0)
	{
		if(m_aSuperSmash[ClientId])
			m_aSuperSmash[ClientId]->m_Number += 1;
		else
			m_aSuperSmash[ClientId] = std::make_unique<CParticleCircle>(&GameServer()->m_World, ClientId, Amount);
	}
	else
	{
		if(m_aSuperSmash[ClientId])
		{
			m_aSuperSmash[ClientId]->m_Number -= 1;
			if(m_aSuperSmash[ClientId]->m_Number == 0)
				m_aSuperSmash[ClientId] = nullptr;
		}
	}
}

void CGameControllerTsmash::SetTeeColor(CPlayer *pPlayer)
{
	auto *pCharacter = pPlayer->GetCharacter();
	if(!pCharacter)
		return;
	auto &LastHealth = m_aLastHealth[pPlayer->GetCid()];
	if(LastHealth == pCharacter->Health())
		return;
	LastHealth = pCharacter->Health();
	// Get color
	ColorHSLA Color;
	if(IsTeamPlay() && pCharacter->Team() == TEAM_RED)
	{
		static constexpr ColorRGBA RED = ColorRGBA(1.0f, 0.5f, 0.5f);
		Color = color_cast<ColorHSLA>(RED);
		Color.h += (float)(10 - pCharacter->Health()) / 10.0f * 0.3f;
	}
	else if(IsTeamPlay() && pCharacter->Team() == TEAM_BLUE)
	{
		static constexpr ColorRGBA BLUE = ColorRGBA(0.7f, 0.7f, 1.0f);
		Color = color_cast<ColorHSLA>(BLUE);
		Color.h -= (float)(10 - pCharacter->Health()) / 10.0f * 0.3f;
	}
	else
	{
		Color = ColorHSLA((float)(10 - pCharacter->Health()) / 10.0f * 0.75f + 0.25f, 1.0f, 0.5f);
	}

	int ColorVal = Color.Pack();
	pPlayer->m_SkinInfoManager.SetUseCustomColor(ESkinPrio::LOW, true);
	pPlayer->m_SkinInfoManager.SetColorBody(ESkinPrio::LOW, ColorVal);
	pPlayer->m_SkinInfoManager.SetColorFeet(ESkinPrio::LOW, ColorVal);
}

CGameControllerTsmash::CGameControllerTsmash(class CGameContext *pGameServer, bool Teams) :
	CGameControllerVanilla(pGameServer)
{
	m_pGameType = Teams ? "TTSmash2" : "TSmash2";
	m_GameFlags = Teams ? GAMEFLAG_TEAMS : 0;

	m_DefaultWeapon = WEAPON_HAMMER;

	m_pStatsTable = Teams ? "ttsmash" : "tsmash";
	m_pExtraColumns = nullptr;
	m_pSqlStats->SetExtraColumns(m_pExtraColumns);
	m_pSqlStats->CreateTable(m_pStatsTable);
}

CGameControllerTsmash::~CGameControllerTsmash() = default;

void CGameControllerTsmash::OnCharacterDeathImpl(CCharacter *pVictim, int Killer, int Weapon, bool SendKillMsg)
{
	// If this is a "self kill" or rather a spike kill
	if(Killer == -1 || Killer == pVictim->GetPlayer()->GetCid())
	{
		const auto &LastToucher = pVictim->GetPlayer()->m_LastToucher;
		if(LastToucher.has_value())
		{
			AddTeamscore(LastToucher->m_Team, 1);
			if(LastToucher->m_ClientId >= 0 && LastToucher->m_ClientId < (int)std::size(GameServer()->m_apPlayers) && GameServer()->m_apPlayers[LastToucher->m_ClientId])
			{
				Killer = LastToucher->m_ClientId;
				Weapon = WEAPON_WORLD;
			}
		}
	}

	CGameControllerVanilla::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
}

int CGameControllerTsmash::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// If this was a "self kill", change the weapon so scoring still happens
	int ModeSpecial = CGameControllerVanilla::OnCharacterDeath(pVictim, pKiller, Weapon == WEAPON_WORLD ? WEAPON_HAMMER : Weapon);
	// On spree, reset health and get some armor so that the spree can go on
	if(g_Config.m_SvKillingspreeKills > 0 && pKiller && pKiller->GetCharacter() && pKiller->Spree() % g_Config.m_SvKillingspreeKills == 0)
	{
		pKiller->GetCharacter()->SetHealth(10);
		pKiller->GetCharacter()->AddArmor(5);
	}
	if(pVictim)
		m_aSuperSmash[pVictim->GetPlayer()->GetCid()] = nullptr;
	return ModeSpecial;
}

void CGameControllerTsmash::OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon)
{
	DoSpikeKillSound(pVictim ? pVictim->GetCid() : -1, pKiller ? pKiller->GetCid() : -1);
	CGameControllerBasePvp::OnKill(pVictim, pKiller, Weapon);
}

void CGameControllerTsmash::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerBasePvp::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);

	m_aSuperSmash[pChr->GetPlayer()->GetCid()] = nullptr;

	m_aLastHealth[pChr->GetPlayer()->GetCid()] = -1;
	SetTeeColor(pChr->GetPlayer());
}

void CGameControllerTsmash::Tick()
{
	for(auto *pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;
		SetTeeColor(pPlayer);
		pPlayer->ResetLastToucherAfterSeconds(5);
	}
	CGameControllerBasePvp::Tick();
}

void CGameControllerTsmash::OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	auto *pKiller = GetPlayerOrNullptr(From);
	// Set last toucher
	if(pKiller)
		pCharacter->GetPlayer()->UpdateLastToucher(pKiller->GetCid(), Weapon);
	// Everything does 1 dmg
	Dmg = 1;
	// Check for super smash
	bool SuperSmash = false;
	if(pKiller && m_aSuperSmash[pKiller->GetCid()])
	{
		GiveSuperSmash(pKiller->GetCid(), -1);
		CNetEvent_Explosion *pEvent = GameServer()->m_Events.Create<CNetEvent_Explosion>(pCharacter->TeamMask());
		if(pEvent)
		{
			pEvent->m_X = (int)pCharacter->GetPos().x;
			pEvent->m_Y = (int)pCharacter->GetPos().y;
		}
		SuperSmash = true;
	}
	// Calculate knockback increase
	float Scale = 1.0f;
	Scale += (10.0f - (float)pCharacter->Health()) / 2.0f; // Low HP = more knockback
	Scale -= (float)pCharacter->Armor() / 10.0f * 0.5f; // High Armor = less knockback
	if(SuperSmash)
		Scale += 15.0f;
	Force *= Scale;
}

bool CGameControllerTsmash::DecreaseHealthAndKill(int Dmg, int From, int Weapon, CCharacter *pCharacter)
{
	if(Dmg == 0)
		return false;
	// Everything does 1 dmg and doesn't kill
	if(pCharacter->Armor() > 0)
		pCharacter->AddArmor(-1);
	else if(pCharacter->Health() > 0)
		pCharacter->AddHealth(-1);
	return false;
}

bool CGameControllerTsmash::OnSelfkill(int ClientId)
{
	GameServer()->SendChatTarget(ClientId, "Self kill is disabled");
	return true;
}

REGISTER_GAMEMODE(tsmash, CGameControllerTsmash(pGameServer, false));
REGISTER_GAMEMODE(ttsmash, CGameControllerTsmash(pGameServer, true));
