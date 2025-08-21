#include <base/color.h>

#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/entities/ddnet_pvp/vanilla_pickup.h>
#include <game/server/gamemodes/base_pvp/base_pvp.h>

#include "tsmash.h"

class CParticleCircle : public CEntity
{
private:
	int m_aParticles[5];
	static constexpr float RADIUS = 30.0f;
	const int m_ClientId;

public:
	CParticleCircle(CGameWorld *pGameWorld, int ClientId) :
		CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE, vec2(0.0f, 0.0f), RADIUS), m_ClientId(ClientId)
	{
		for(int &Particle : m_aParticles)
			Particle = Server()->SnapNewId();
		m_Number = 1;
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

static int ColorToSixup(int Color6)
{
	return ColorHSLA(Color6).UnclampLighting(ColorHSLA::DARKEST_LGT).Pack(ColorHSLA::DARKEST_LGT7);
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
	if(m_GameFlags & GAMEFLAG_TEAMS && pCharacter->Team() == TEAM_RED)
	{
		static constexpr ColorRGBA RED = ColorRGBA(1.0f, 0.5f, 0.5f);
		Color = color_cast<ColorHSLA>(RED);
		Color.h += (float)(10 - pCharacter->Health()) / 10.0f * 0.3f;
	}
	else if(m_GameFlags & GAMEFLAG_TEAMS && pCharacter->Team() == TEAM_BLUE)
	{
		static constexpr ColorRGBA BLUE = ColorRGBA(0.7f, 0.7f, 1.0f);
		Color = color_cast<ColorHSLA>(BLUE);
		Color.h -= (float)(10 - pCharacter->Health()) / 10.0f * 0.3f;
	}
	else
	{
		Color = ColorHSLA((float)(10 - pCharacter->Health()) / 10.0f * 0.75f + 0.25f, 1.0f, 0.5f);
	}
	// 0.6
	auto Color6 = Color.Pack();
	pPlayer->m_TeeInfos.m_UseCustomColor = 1;
	pPlayer->m_TeeInfos.m_ColorBody = Color6;
	pPlayer->m_TeeInfos.m_ColorFeet = Color6;
	// 0.7
	for(bool &UseCustomColor : pPlayer->m_TeeInfos.m_aUseCustomColors)
		UseCustomColor = true;
	const int Color7 = ColorToSixup(Color6);
	for(int &PartColor : pPlayer->m_TeeInfos.m_aSkinPartColors)
		PartColor = Color7;
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
		if(LastToucher.has_value() && LastToucher->m_ClientId >= 0 && LastToucher->m_ClientId < (int)std::size(GameServer()->m_apPlayers) && GameServer()->m_apPlayers[LastToucher->m_ClientId])
		{
			Killer = LastToucher->m_ClientId;
			Weapon = WEAPON_WORLD;
		}
	}

	CGameControllerVanilla::OnCharacterDeathImpl(pVictim, Killer, Weapon, SendKillMsg);
}

int CGameControllerTsmash::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	// If this was a "self kill", change the weapon so scoring still happens
	int ModeSpecial = CGameControllerVanilla::OnCharacterDeath(pVictim, pKiller, Weapon == WEAPON_WORLD ? WEAPON_HAMMER : Weapon);
	// Do spree stuff
	if(pKiller)
	{
		if(g_Config.m_SvKillingspreeKills > 0 && pKiller->Spree() % g_Config.m_SvKillingspreeKills == 0)
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Due to their spree, '%s' will have a super hammer for their next hit!", Server()->ClientName(pKiller->GetCid()));
			GameServer()->SendChat(-1, TEAM_ALL, aBuf);
			if(m_SuperSmash[pKiller->GetCid()])
				m_SuperSmash[pKiller->GetCid()]->m_Number += 1;
			else
				m_SuperSmash[pKiller->GetCid()] = std::make_unique<CParticleCircle>(&GameServer()->m_World, pKiller->GetCid());
		}
	}
	if(pVictim)
		m_SuperSmash.erase(pVictim->GetPlayer()->GetCid());
	return ModeSpecial;
}

void CGameControllerTsmash::OnKill(class CPlayer *pVictim, class CPlayer *pKiller, int Weapon)
{
	DoSpikeKillSound(pVictim ? pVictim->GetCid() : -1, pKiller ? pKiller->GetCid() : -1);
	CGameControllerPvp::OnKill(pVictim, pKiller, Weapon);
}

void CGameControllerTsmash::OnCharacterSpawn(class CCharacter *pChr)
{
	CGameControllerPvp::OnCharacterSpawn(pChr);

	// give default weapons
	pChr->GiveWeapon(m_DefaultWeapon, false, -1);

	m_SuperSmash.erase(pChr->GetPlayer()->GetCid());

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
	}
	CGameControllerPvp::Tick();
}

void CGameControllerTsmash::OnAnyDamage(vec2 &Force, int &Dmg, int &From, int &Weapon, CCharacter *pCharacter)
{
	Dmg = 1;
	float Scale = 1.0f;
	Scale += (10.0f - (float)pCharacter->Health()) / 2.0f; // Low HP = more knockback
	Scale -= (float)pCharacter->Armor() / 10.0f * 0.5f; // High Armor = less knockback
	auto It = m_SuperSmash.find(From);
	if(It != m_SuperSmash.end())
	{
		It->second->m_Number -= 1;
		if(It->second->m_Number == 0)
			m_SuperSmash.erase(It);
		Scale += 15.0f;
		CNetEvent_Explosion *pEvent = GameServer()->m_Events.Create<CNetEvent_Explosion>(pCharacter->TeamMask());
		if(pEvent)
		{
			pEvent->m_X = (int)pCharacter->GetPos().x;
			pEvent->m_Y = (int)pCharacter->GetPos().y;
		}
	}
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
