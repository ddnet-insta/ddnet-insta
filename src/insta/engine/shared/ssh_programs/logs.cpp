#if defined(CONF_SSH)

#include "logs.h"

#include <base/logger.h>

#include <insta/engine/shared/ssh_server.h>
#include <libssh/libssh.h>

void CSshProgramLogs::OnInit()
{
	// hide cursor
	ssh_channel_write(m_pClient->m_Channel, "\033[?25l", 7);
}

void CSshProgramLogs::OnShutdown()
{
	// show cursor
	ssh_channel_write(m_pClient->m_Channel, "\033[?25h", 7);
}

void CSshProgramLogs::OnLogMessage(const CLogMessage *pMessage)
{
	if(pMessage->m_Level > IConsole::ToLogLevelFilter(g_Config.m_ConsoleOutputLevel))
		return;

	m_pClient->SendLogLine(pMessage);
}

#endif
