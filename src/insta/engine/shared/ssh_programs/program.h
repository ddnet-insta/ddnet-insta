#ifndef INSTA_ENGINE_SHARED_SSH_PROGRAMS_PROGRAM_H
#define INSTA_ENGINE_SHARED_SSH_PROGRAMS_PROGRAM_H

#if defined(CONF_SSH)

#include <base/logger.h>

class CSshClient;
class CSshServer;
class CByteBuffer;

class CSshProgram
{
protected:
	CSshClient *m_pClient = nullptr;

public:
	CSshProgram(CSshClient *pClient);
	virtual ~CSshProgram() = default;

	virtual void OnLogMessage(const CLogMessage *pMessage) {}
	virtual void TryProcessCurrentInput(CByteBuffer *pBuffer) {}

	// return false to ignore sigint and keep running
	virtual bool OnSigint() { return true; }

	virtual void OnInit() {}
	virtual void OnShutdown() {}
};

#endif

#endif
