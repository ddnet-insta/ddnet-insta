#ifndef ENGINE_INSTAGIB_GAMESERVER_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_ENGINE_GAMESERVER

class IGameServer : public IInterface
{
#endif // IN_CLASS_ENGINE_GAMESERVER
public:
	virtual const char *ServerInfoClientScoreKind() = 0;
	virtual bool OnClientPacket(int ClientId, bool Sys, int MsgId, struct CNetChunk *pPacket, class CUnpacker *pUnpacker) = 0;

#ifndef IN_CLASS_ENGINE_GAMESERVER
};
#endif
