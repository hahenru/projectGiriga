#ifndef GIRIGARA_CORE_CORE_CORECONTEXT_H
#define GIRIGARA_CORE_CORE_CORECONTEXT_H

#include <memory>
#include <System/LoggerSystem.h>
#include <System/ConsoleSystem.h>
#include <Event/EventBus.h>
#include <Script/ScriptEngine.h>
#include <System/SettingsRegistry.h>
#include <Resource/IResourceManager.h>
#include <System/FileSystem.h>
#include <System/ThreadPool.h>

class CoreContext {
public:
    CoreContext();
    virtual ~CoreContext();

    CoreContext(const CoreContext&) = delete;
    CoreContext& operator=(const CoreContext&) = delete;

	std::unique_ptr<ThreadPool> threadPool;
    std::unique_ptr<EventBus> eventBus;
    std::unique_ptr<LoggerSystem> logger;
    std::unique_ptr<ScriptEngine> scriptEngine;
    std::unique_ptr<ConsoleSystem> console;
    std::unique_ptr<SettingsRegistry> settings;
    std::unique_ptr<FileSystem> fileSystem;

    // 資源管理的多型基底指針 (由 Girigara 依無頭/圖形模式掛載實體)
    IResourceManager* resources{nullptr};
};

#endif // GIRIGARA_CORE_CORE_CORECONTEXT_H