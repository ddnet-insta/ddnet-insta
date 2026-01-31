#ifndef INSTA_SERVER_ANTIBOB_H
#define INSTA_SERVER_ANTIBOB_H

class IConsole;

class CAntibobContext
{
public:
	IConsole *m_pConsole = nullptr;
};

extern CAntibobContext g_AntibobContext;

extern "C" {

int AntibobVersion();
void AntibobRcon(const char *pLine);
}

#endif
