#ifndef ENGINE_INSTAGIB_SERVER_H
// hack for headerguard linter
#endif

#ifndef IN_CLASS_ENGINE_SERVER

class IServer : public IInterface
{
#endif // IN_CLASS_ENGINE_SERVER
public:
	virtual void AddMapToRandomPool(const char *pMap) = 0;
	virtual void ClearRandomMapPool() = 0;
	virtual const char *GetRandomMapFromPool() = 0;
	// ddnet-insta method that force stops the server
	virtual void ShutdownServer() = 0;
	// called when a 0.7 player sends rcon credentials
	// returns true if these were in the format username:password
	// and matched some ddnet rcon user
	// in that case the player will also be logged in
	// returns false otherwise
	virtual bool SixupUsernameAuth(int ClientId, const char *pCredentials) = 0;
	virtual CAuthManager *AuthManager() = 0;
	virtual bool HasShowIpsOn(int ClientId) const = 0;

private:
#ifndef IN_CLASS_ENGINE_SERVER
};
#endif
