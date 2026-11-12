#ifndef GIRIGARA_INCLUDE_NETWORK_ENETNODE_H
#define GIRIGARA_INCLUDE_NETWORK_ENETNODE_H
#include <enet/enet.h>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

class ENetNode {
public:
    using OnConnectCallback    = std::function<void(uint32_t peerId)>;
    using OnDisconnectCallback = std::function<void(uint32_t peerId)>;
    using OnReceiveCallback    = std::function<void(uint32_t peerId, uint8_t channel, const uint8_t* data, size_t size)>;

    ENetNode() = default;
    ~ENetNode();

    ENetNode(const ENetNode&) = delete;
    ENetNode& operator=(const ENetNode&) = delete;

    // 啟動為伺服器模式 (監聽指定 Port)
    bool startServer(uint16_t port, size_t maxClients = 32, size_t channels = 2);

    // 啟動為客戶端模式並連線至目標
    bool connectClient(const std::string& host, uint16_t port, size_t channels = 2);

    // 關閉節點並釋放 ENetHost
    void shutdown();

    // 非阻塞輪詢事件 (在主迴圈每個 Tick 呼叫)
    void poll(uint32_t timeoutMs = 0);

    // 發送封包給指定 Peer (Client 模式下 peerId 預設為 0 即伺服器)
    void send(uint32_t peerId, uint8_t channel, const void* data, size_t size, bool reliable = true);

    // 廣播封包給所有已連線的 Peer (Server 模式專用)
    void broadcast(uint8_t channel, const void* data, size_t size, bool reliable = true);

    void setOnConnect(OnConnectCallback cb) { m_onConnect = std::move(cb); }
    void setOnDisconnect(OnDisconnectCallback cb) { m_onDisconnect = std::move(cb); }
    void setOnReceive(OnReceiveCallback cb) { m_onReceive = std::move(cb); }

private:
    ENetHost* m_host{nullptr};
    std::vector<ENetPeer*> m_peers; // 簡單的索引表管理 Peer

    OnConnectCallback m_onConnect;
    OnDisconnectCallback m_onDisconnect;
    OnReceiveCallback m_onReceive;
};
#endif /* GIRIGARA_INCLUDE_NETWORK_ENETNODE_H */