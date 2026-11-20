#include <Core/CoreContext.h>

CoreContext::CoreContext()
: threadPool(std::make_unique<ThreadPool>(0)),
eventBus(std::make_unique<EventBus>()),
logger(std::make_unique<LoggerSystem>(*eventBus, 65535)),
scriptEngine(std::make_unique<ScriptEngine>(*eventBus)),
console(std::make_unique<ConsoleSystem>(*logger)),
#ifndef GIRIGARA_SERVER
settings(std::make_unique<SettingsRegistry>(*eventBus, *logger)),
#endif
fileSystem(std::make_unique<FileSystem>(*eventBus, *logger)) {

    console->setScriptEngine(scriptEngine.get());
    logger->info("[CoreContext] 引擎核心基礎子系統初始化完畢。");
}

CoreContext::~CoreContext() {
    threadPool.reset();
    console.reset();
    #ifndef GIRIGARA_SERVER
    settings.reset();
    #endif
    fileSystem.reset();
    if (logger) {
        logger->info("[CoreContext] 核心基礎子系統已釋放。");
        logger.reset();
    }
    scriptEngine.reset();
    eventBus.reset();
}
