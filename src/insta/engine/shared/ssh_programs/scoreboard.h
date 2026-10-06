#ifndef INSTA_ENGINE_SHARED_SSH_PROGRAMS_SCOREBOARD_H
#define INSTA_ENGINE_SHARED_SSH_PROGRAMS_SCOREBOARD_H

#if defined(CONF_SSH)

#include <base/logger.h>

#include <insta/engine/shared/ssh_programs/program.h>

class CSshClient;
class CSshServer;

class CSshProgramScoreboard : public CSshProgram
{
	// TODO: not sure yet if inheriting constructor is annoying
	//       maybe a init and shutdown method would be better anyways
	using CSshProgram::CSshProgram;

	char m_aLastSendScoreboard[262144] = "";

	void BuildScoreboardStr(char *pBuf, int BufSize);

	void RenderScoreboard();

public:
	void OnInit() override;
	void OnShutdown() override;
	void OnTick() override;
};

#endif

#endif
