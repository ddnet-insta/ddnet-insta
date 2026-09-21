#ifndef INSTA_ENGINE_SHARED_SSH_PROGRAMS_LOGS_H
#define INSTA_ENGINE_SHARED_SSH_PROGRAMS_LOGS_H

#if defined(CONF_SSH)

#include <base/logger.h>

#include <insta/engine/shared/ssh_programs/program.h>

class CSshClient;
class CSshServer;

class CSshProgramLogs : public CSshProgram
{
	// TODO: not sure yet if inheriting constructor is annoying
	//       maybe a init and shutdown method would be better anyways
	using CSshProgram::CSshProgram;

public:
	void OnInit() override;
	void OnShutdown() override;
	void OnLogMessage(const CLogMessage *pMessage) override;
};

#endif

#endif
