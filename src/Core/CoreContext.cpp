#include <Core/CoreContext.h>

CoreContext::CoreContext()
    : logger(std::make_unique<LoggerSystem>(65535)),
      eventBus(std::make_unique<EventBus>()),
      scriptEngine(std::make_unique<ScriptEngine>(*eventBus)),
      console(std::make_unique<ConsoleSystem>(*logger)),
      settings(std::make_unique<SettingsRegistry>(*logger)) {

    console->setScriptEngine(scriptEngine.get());

    logger->info("[CoreContext] 雙端共用核心子系統初始化完畢。");
}

CoreContext::~CoreContext() {
    // 逆序釋放純邏輯子系統
    console.reset();
    scriptEngine.reset();
    settings.reset();
    eventBus.reset();
    if (logger) {
        logger->info("[CoreContext] 核心子系統已釋放。");
        logger.reset();
    }
}
