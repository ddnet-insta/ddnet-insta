#ifndef GAME_SERVER_INSTAGIB_ANTIBOB_H
#define GAME_SERVER_INSTAGIB_ANTIBOB_H

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
