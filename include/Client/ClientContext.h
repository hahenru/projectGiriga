#ifndef GIRIGARA_CLIENT_CLIENT_CONTEXT_H
#define GIRIGARA_CLIENT_CLIENT_CONTEXT_H

#include <Core/CoreContext.h>
#include <Window/GlfwContext.h>
#include <Window/WindowManager.h>
#include <System/KeyboardSystem.h>
#include <Client/ClientResourceManager.h>
#include <Audio/AudioSystem.h>
#include <Network/ENetContext.h>
#include <Network/ENetNode.h>


class ClientContext : public CoreContext {
public:
    ClientContext(int width, int height, const std::string& title);
    ~ClientContext() override;

    std::unique_ptr<GlfwContext> glfw;
    std::unique_ptr<WindowManager> window;
    std::unique_ptr<AudioSystem> audio;
    std::unique_ptr<KeyboardSystem> keyboard;
    std::unique_ptr<ClientResourceManager> clientResources;
	std::unique_ptr<ENetContext> enetCtx;
    std::unique_ptr<ENetNode> network;
};

#endif // GIRIGARA_CLIENT_CLIENT_CONTEXT_H