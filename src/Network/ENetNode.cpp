#include <Network/ENetNode.h>
#include <iostream>
#include <System/LoggerSystem.h>

ENetNode::ENetNode(LoggerSystem& logger)
    : m_logger(logger) {}

ENetNode::~ENetNode() {
    shutdown();
}

bool ENetNode::startServer(uint16_t port, size_t maxClients, size_t channels) {
    shutdown();

    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;

    m_host = enet_host_create(&address, maxClients, channels, 0, 0);
    if (!m_host) {
        std::cerr << "[ENetNode] Failed to create ENet server host on port " << port << "\n";
        return false;
    }
    m_peers.resize(maxClients, nullptr);
    return true;
}

bool ENetNode::connectClient(const std::string& host, uint16_t port, size_t channels) {
    shutdown();

    m_host = enet_host_create(nullptr, 1, channels, 0, 0);
    if (!m_host) {
        std::cerr << "[ENetNode] Failed to create ENet client host.\n";
        return false;
    }

    ENetAddress address{};
    enet_address_set_host(&address, host.c_str());
    address.port = port;

    ENetPeer* serverPeer = enet_host_connect(m_host, &address, channels, 0);
    if (!serverPeer) {
        std::cerr << "[ENetNode] No available peers for initiating connection.\n";
        shutdown();
        return false;
    }

    m_peers.resize(1, nullptr);
    m_peers[0] = serverPeer;
    return true;
}

void ENetNode::shutdown() {
    if (m_host) {
        enet_host_destroy(m_host);
        m_host = nullptr;
    }
    m_peers.clear();
}

void ENetNode::poll(uint32_t timeoutMs) {
    if (!m_host) return;

    ENetEvent event{};
    while (enet_host_service(m_host, &event, timeoutMs) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                uint32_t assignedId = 0;
                // 若為 Server，找空位配置 peerId；若為 Client，m_peers[0] 已經是 serverPeer
                if (m_peers.size() > 1) {
                    for (uint32_t i = 0; i < m_peers.size(); ++i) {
                        if (m_peers[i] == nullptr) {
                            m_peers[i] = event.peer;
                            assignedId = i;
                            break;
                        }
                    }
                }
                event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(assignedId));
                if (m_onConnect) m_onConnect(assignedId);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                uint32_t peerId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event.peer->data));
                if (m_onReceive) {
                    m_onReceive(peerId, event.channelID, event.packet->data, event.packet->dataLength);
                }
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                uint32_t peerId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(event.peer->data));
                if (peerId < m_peers.size()) {
                    m_peers[peerId] = nullptr;
                }
                event.peer->data = nullptr;
                if (m_onDisconnect) m_onDisconnect(peerId);
                break;
            }
            default:
                break;
        }
    }
}

void ENetNode::send(uint32_t peerId, uint8_t channel, const void* data, size_t size, bool reliable) {
    if (!m_host || peerId >= m_peers.size() || !m_peers[peerId]) return;

    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    enet_peer_send(m_peers[peerId], channel, packet);
}

void ENetNode::broadcast(uint8_t channel, const void* data, size_t size, bool reliable) {
    if (!m_host) return;

    enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    enet_host_broadcast(m_host, channel, packet);
}