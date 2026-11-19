// src/Core/CoreContext.cpp
#include <Core/CoreContext.h>

CoreContext::CoreContext()
    : threadPool(std::make_unique<ThreadPool>(0)),
	  eventBus(std::make_unique<EventBus>()),
      logger(std::make_unique<LoggerSystem>(*eventBus, 65535)),
      scriptEngine(std::make_unique<ScriptEngine>(*eventBus)),
      console(std::make_unique<ConsoleSystem>(*logger)),
      settings(std::make_unique<SettingsRegistry>(*eventBus, *logger)),
      fileSystem(std::make_unique<FileSystem>(*eventBus, *logger)) {

    console->setScriptEngine(scriptEngine.get());
    logger->info("[CoreContext] 引擎核心基礎子系統初始化完畢。");
}

CoreContext::~CoreContext() {
	threadPool.reset();
    console.reset();
    settings.reset();
    fileSystem.reset();
    if (logger) {
        logger->info("[CoreContext] 核心基礎子系統已釋放。");
        logger.reset();
    }
    scriptEngine.reset();
    eventBus.reset();
}