#ifndef GIRIGARA_SYSTEM_LOGGER_SYSTEM_H
#define GIRIGARA_SYSTEM_LOGGER_SYSTEM_H

#include <Core/Identifiable.h>
#include <Event/EventBus.h>
#include <string>
#include <deque>
#include <cstddef>

enum class LogLevel {
    Info,
    Warning,
    Error
};

class LoggerSystem : public Identifiable {
public:
    explicit LoggerSystem(EventBus& eventBus, std::size_t maxHistoryLines = 65535);
    ~LoggerSystem() override;

    LoggerSystem(const LoggerSystem&) = delete;
    LoggerSystem& operator=(const LoggerSystem&) = delete;

    // 核心 Log 函式
    void log(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
    void clear();

    const std::deque<std::string>& getLogs() const { return m_logs; }
    bool isDirty() const { return m_dirty; }
    void clearDirty() { m_dirty = false; }

private:
    void registerScriptBindings();
    void unregisterScriptBindings();

    EventBus& m_eventBus;
    std::deque<std::string> m_logs;
    std::size_t m_maxHistoryLines;
    bool m_dirty{true};
};

#endif // GIRIGARA_SYSTEM_LOGGER_SYSTEM_H
