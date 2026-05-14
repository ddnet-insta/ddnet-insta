#ifndef INSTA_ENGINE_SERVER_H
#define INSTA_ENGINE_SERVER_H
#undef INSTA_ENGINE_SERVER_H
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

	// Create a tee from the server side without a actual client connecting to it.
	// Creates a full player and character instance and uses up a slot.
	// Returns -1 on error and the new ClientId otherwise.
	//
	// WARNING: avoid calling this method. Use the IGameController::CreateTee() wrapper instead.
	virtual int CreateTee(const char *pName) = 0;
	virtual void DropTee(int ClientId) = 0;
	virtual bool IsDebugDummy(int ClientId) const = 0;

private:
#ifndef IN_CLASS_ENGINE_SERVER
};
#endif
