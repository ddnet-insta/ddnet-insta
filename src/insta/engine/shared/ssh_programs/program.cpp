#if defined(CONF_SSH)

#include "program.h"

#include <insta/engine/shared/ssh_server.h>

CSshProgram::CSshProgram(CSshClient *pClient)
{
	m_pClient = pClient;
}

#endif
