#ifndef GIRIGARA_INCLUDE_NETWORK_ENETCONTEXT_H
#define GIRIGARA_INCLUDE_NETWORK_ENETCONTEXT_H

#include <enet/enet.h>
#include <stdexcept>
#include <atomic>

class ENetContext {
public:
    ENetContext() {
        if (s_refCount.fetch_add(1) == 0) {
            if (enet_initialize() != 0) {
                s_refCount.fetch_sub(1);
                throw std::runtime_error("[ENetContext] Failed to initialize ENet.");
            }
        }
    }

    ~ENetContext() {
        if (s_refCount.fetch_sub(1) == 1) {
            enet_deinitialize();
        }
    }

    ENetContext(const ENetContext&) = delete;
    ENetContext& operator=(const ENetContext&) = delete;

private:
    static inline std::atomic<int> s_refCount{0};
};

#endif /* GIRIGARA_INCLUDE_NETWORK_ENETCONTEXT_H */