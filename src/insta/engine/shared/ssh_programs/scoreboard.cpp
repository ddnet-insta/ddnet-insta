#if defined(CONF_SSH)

#include "scoreboard.h"

#include <base/logger.h>
#include <base/str.h>

#include <engine/shared/protocol.h>

#include <insta/engine/shared/ssh_server.h>
#include <libssh/libssh.h>

void CSshProgramScoreboard::OnInit()
{
	// hide cursor
	ssh_channel_write(m_pClient->m_Channel, "\033[?25l", 7);
}

void CSshProgramScoreboard::OnShutdown()
{
	// show cursor
	ssh_channel_write(m_pClient->m_Channel, "\033[?25h", 7);
}

void CSshProgramScoreboard::BuildScoreboardStr(char *pBuf, int BufSize)
{
	char aRow[8192] = "";
	int Width = std::min(m_pClient->m_Term.m_Width - 2,
		static_cast<int>(sizeof(aRow) - 4));
	int Height = m_pClient->m_Term.m_Height - 2;
	pBuf[0] = '\0';
	if(Width < 10 || Height < 6)
	{
		// TODO: log some error about space here somewhere
		return;
	}

	memset(aRow, '-', Width);
	aRow[0] = '+';
	aRow[Width - 1] = '+';
	aRow[Width] = '\0';
	str_append(pBuf, aRow, BufSize);
	str_append(pBuf, "\n\r", BufSize);

	int NumRows = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_CallbackCtx.m_pServer->Server()->ClientIngame(i))
			continue;
		if(NumRows++ >= Height)
			break;

		const char *pName = m_pClient->m_CallbackCtx.m_pServer->Server()->ClientName(i);

		int NameColWidth = Width - 4;

		str_format(
			aRow,
			sizeof(aRow),
			"| %-*s |\n\r",
			NameColWidth,
			pName);
		str_append(pBuf, aRow, BufSize);
	}

	memset(aRow, '-', Width);
	aRow[0] = '+';
	aRow[Width - 1] = '+';
	aRow[Width] = '\0';
	str_append(pBuf, aRow, BufSize);
}

void CSshProgramScoreboard::RenderScoreboard()
{
	char aNewScoreboard[262144];
	BuildScoreboardStr(aNewScoreboard, sizeof(aNewScoreboard));
	if(str_comp(aNewScoreboard, m_aLastSendScoreboard))
	{
		str_copy(m_aLastSendScoreboard, aNewScoreboard);
		ssh_channel_write(m_pClient->m_Channel, "\033[2J\r\033[H", 9);
		ssh_channel_write(m_pClient->m_Channel, aNewScoreboard, str_length(aNewScoreboard));
	}
}

void CSshProgramScoreboard::OnTick()
{
	RenderScoreboard();
}

#endif
