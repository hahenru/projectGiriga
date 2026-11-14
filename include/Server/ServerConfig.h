#ifndef GIRIGARA_SERVER_CONFIG_H
#define GIRIGARA_SERVER_CONFIG_H

#include <cstdint>
#include <cstddef>
#include <string>

struct ServerConfig {
    uint16_t port = 25565;
    int tickRate = 20;
    size_t maxClients = 32;

    static ServerConfig loadOrCreate(const std::string& path = "server.cfg");
};

#endif // GIRIGARA_SERVER_CONFIG_H
