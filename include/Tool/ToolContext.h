#ifndef GIRIGARA_TOOL_TOOL_CONTEXT_H
#define GIRIGARA_TOOL_TOOL_CONTEXT_H

#include <Core/CoreContext.h>
#include <Window/GlfwContext.h>
#include <Window/WindowManager.h>
#include <System/KeyboardSystem.h>
#include <Tool/ToolResourceManager.h>
#include <Script/GpkScriptBinder.h>

class ToolContext : public CoreContext {
public:
    ToolContext(int width, int height, const std::string& title);
    ~ToolContext() override;

    std::unique_ptr<GlfwContext> glfw;
    std::unique_ptr<WindowManager> window;
    std::unique_ptr<KeyboardSystem> keyboard;
    std::unique_ptr<ToolResourceManager> toolResources;
    std::unique_ptr<GpkScriptBinder> gpkBinder;
};

#endif // GIRIGARA_TOOL_TOOL_CONTEXT_H
