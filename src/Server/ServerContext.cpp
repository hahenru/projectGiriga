#include <Server/ServerContext.h>

ServerContext::ServerContext()
    : CoreContext(),
	serverResources(std::make_unique<ServerResourceManager>(*logger)),
    enetCtx(std::make_unique<ENetContext>()),
    network(std::make_unique<ENetNode>(*logger)){

    // ★ 關鍵：將伺服器純文本管理器掛給基底抽象指標
    this->resources = serverResources.get();

    logger->info("[ServerContext] 無頭專用伺服器環境已就緒 (零圖形與音訊依賴)。");
}

ServerContext::~ServerContext() {
	// 嚴格逆序釋放：先銷毀 network，再釋放 enetCtx (呼叫 enet_deinitialize)
    network.reset();
    enetCtx.reset();
    serverResources.reset();
}