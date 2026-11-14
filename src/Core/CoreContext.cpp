#include <Core/CoreContext.h>

CoreContext::CoreContext()
    : eventBus(std::make_unique<EventBus>()),
      logger(std::make_unique<LoggerSystem>(*eventBus, 65535)),
      scriptEngine(std::make_unique<ScriptEngine>(*eventBus)),
      console(std::make_unique<ConsoleSystem>(*logger)),
      settings(std::make_unique<SettingsRegistry>(*eventBus, *logger)) {

    console->setScriptEngine(scriptEngine.get());

    logger->info("[CoreContext] 雙端共用核心子系統初始化完畢。");
}

CoreContext::~CoreContext() {
    // 逆序釋放純邏輯子系統（EventBus 必須留到最後銷毀）
    console.reset();
    settings.reset();
    if (logger) {
        logger->info("[CoreContext] 核心子系統已釋放。");
        logger.reset();
    }
    scriptEngine.reset();
    eventBus.reset();
}
