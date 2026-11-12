#include <Server/ServerContext.h>

ServerContext::ServerContext()
    : CoreContext(),
      serverResources(std::make_unique<ServerResourceManager>(*logger)) {

    // ★ 關鍵：將伺服器純文本管理器掛給基底抽象指標
    this->resources = serverResources.get();

    logger->info("[ServerContext] 無頭專用伺服器環境已就緒 (零圖形與音訊依賴)。");
}

ServerContext::~ServerContext() {
    serverResources.reset();
}