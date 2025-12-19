#include "text.h"

#include <engine/server.h>

CText::CText(CGameWorld *pGameWorld, CClientMask Mask, vec2 Pos, int AliveTicks, const char *pText, int EntType) :
	CEntity(pGameWorld, EntType, Pos)
{
	m_CurTicks = 0;
	m_StartTick = Server()->Tick();
	m_AliveTicks = AliveTicks;
	m_Mask = Mask;

	str_copy(m_aText, pText ? pText : "", sizeof(m_aText));
}

void CText::Reset()
{
	for(const auto *pData : m_pData)
		Server()->SnapFreeId(pData->m_Id);

	m_MarkedForDestroy = true;
}

void CText::Tick()
{
	if(++m_CurTicks - m_StartTick > m_AliveTicks)
		Reset();
}

void CText::SetData(float Cell)
{
	// Horizontal cursor measured in columns
	float XCursorCols = 0;

	for(int Ci = 0; Ci < str_length(m_aText) && Ci < MAX_TEXT_LEN; ++Ci)
	{
		const auto Ch = (unsigned char)m_aText[Ci];

		// Find tight horizontal bounds for the glyph: [MinC, MaxC]
		int MinC = GlyphW; // first column with a pixel
		int MaxC = -1; // last column with a pixel

		for(int r = 0; r < GlyphH; ++r)
		{
			for(int c = 0; c < GlyphW; ++c)
			{
				if(asciiTable[Ch][r][c])
				{
					if(c < MinC)
						MinC = c;
					if(c > MaxC)
						MaxC = c;
				}
			}
		}

		// Handle empty glyphs (no pixels): width 0, but still add gap
		const float TightWidth = (MaxC >= MinC) ? (float)(MaxC - MinC + 1) : 0.0f;

		// Compute vertical offset in cells for this glyph
		const float YOffCells = GlyphYOffset(Ch) - 2.0f;

		// Render only the tight columns
		if(TightWidth > 0.0f)
		{
			for(int r = 0; r < GlyphH; ++r)
			{
				for(int c = MinC; c <= MaxC; ++c)
				{
					if(!asciiTable[Ch][r][c])
						continue;

					const vec2 Pos = m_Pos + vec2((XCursorCols + (float)(c - MinC)) * Cell * 0.70f, ((float)r + YOffCells) * Cell * 0.70f);
					const int EntityId = Server()->SnapNewId();

					auto *pD = new STextData();
					pD->m_Id = EntityId;
					pD->m_Pos = Pos;
					m_pData.push_back(pD);
				}
			}
		}

		// Advance cursor by tight glyph width + per-glyph gap (in columns), then scale uniformly
		XCursorCols += TightWidth + GlyphXGap(Ch);
	}
	m_CenterX = (XCursorCols * Cell * 0.70f) / 2.0f;
}

inline float CText::GlyphYOffset(unsigned char Ch)
{
	switch(Ch)
	{
	case 'g':
	case 'p':
	case 'q':
	case 'j':
	case 'f':
		return 0.85f;
	default:
		return 0.0f;
	}
}

// Horizontal gap per glyph (in columns)
inline float CText::GlyphXGap(unsigned char Ch)
{
	if(Ch == ' ')
		return 1.65f; // bigger gap for space
	return 0.75f; // normal positive gap for letters
}
