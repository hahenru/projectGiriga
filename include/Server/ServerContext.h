#ifndef GIRIGARA_SERVER_SERVER_SERVERCONTEXT_H
#define GIRIGARA_SERVER_SERVER_SERVERCONTEXT_H

#include <Core/CoreContext.h>
#include <Server/ServerResourceManager.h>
#include <Network/ENetContext.h>
#include <Network/ENetNode.h>

class ServerContext : public CoreContext {
public:
    ServerContext();
    ~ServerContext() override;

    std::unique_ptr<ServerResourceManager> serverResources;
	std::unique_ptr<ENetContext> enetCtx;
    std::unique_ptr<ENetNode> network;
};

#endif//GIRIGARA_SERVER_SERVER_SERVERCONTEXT_H