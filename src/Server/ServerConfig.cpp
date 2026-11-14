#include <Server/ServerConfig.h>
#include <fstream>
#include <algorithm>
#include <cctype>

namespace {
    std::string trim(std::string s) {
        auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
        return s;
    }
} // namespace

ServerConfig ServerConfig::loadOrCreate(const std::string& path) {
    ServerConfig cfg;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::ofstream out(path);
        if (out.is_open()) {
            out << "# Project Girigara Dedicated Server Config\n"
            << "port=" << cfg.port << "\n"
            << "tick_rate=" << cfg.tickRate << "\n"
            << "max_clients=" << cfg.maxClients << "\n";
        }
        return cfg;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, pos));
        std::string val = trim(line.substr(pos + 1));

        try {
            if (key == "port")        cfg.port = static_cast<uint16_t>(std::stoul(val));
            else if (key == "tick_rate")   cfg.tickRate = std::stoi(val);
            else if (key == "max_clients") cfg.maxClients = static_cast<size_t>(std::stoul(val));
        } catch (...) {
            // 忽略格式錯誤的欄位，保留預設值
        }
    }
    return cfg;
}
