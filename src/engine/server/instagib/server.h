#ifndef ENGINE_SERVER_INSTAGIB_SERVER_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_ENGINE_SERVER_SERVER

class CServer : public IServer
{
#endif // IN_CLASS_ENGINE_SERVER_SERVER
public:
	std::vector<std::string> m_vMapPool;
	void AddMapToRandomPool(const char *pMap) override;
	void ClearRandomMapPool() override;
	const char *GetRandomMapFromPool() override;
	void ShutdownServer() override { m_RunServer = STOPPING; }
	CAuthManager *AuthManager() override { return &m_AuthManager; }
	static void ConRedirect(IConsole::IResult *pResult, void *pUser);
	bool SixupUsernameAuth(int ClientId, const char *pCredentials) override;
	bool HasShowIpsOn(int ClientId) const override;

private:
#ifndef IN_CLASS_ENGINE_SERVER_SERVER
};
#endif
