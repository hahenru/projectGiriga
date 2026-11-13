#ifndef GIRIGARA_CORE_DEPERSONALIZATION_H
#define GIRIGARA_CORE_DEPERSONALIZATION_H

#include <cstdint>
#include <random>
#include <functional>

class Depersonalization {
public:
    // 預設建構子：自動生成唯一識別碼
    Depersonalization() : m_probands(generate()) {}

    // 指定已知 ID 建構（傳入 0 代表無效/未賦予編號）
    explicit Depersonalization(uint64_t id) : m_probands(id) {}

    Depersonalization(const Depersonalization&) = default;
    Depersonalization& operator=(const Depersonalization&) = default;

    // 支援隱含轉換為 uint64_t，無縫相容 UIRegistry 與序列化介面
    operator uint64_t() const { return m_probands; }
    uint64_t get() const { return m_probands; }

    bool isValid() const { return m_probands != 0; }

    bool operator==(const Depersonalization& other) const { return m_probands == other.m_probands; }
    bool operator!=(const Depersonalization& other) const { return m_probands != other.m_probands; }
    bool operator<(const Depersonalization& other) const { return m_probands < other.m_probands; }

private:
    static uint64_t generate() {
        static thread_local std::mt19937_64 engine(std::random_device{}());
        static thread_local std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);
        return dist(engine);
    }

    uint64_t m_probands{0};
};

// 特化 std::hash，使其支援 std::unordered_map 與 std::unordered_set
namespace std {
    template <>
    struct hash<Depersonalization> {
        std::size_t operator()(const Depersonalization& id) const noexcept {
            return std::hash<uint64_t>()(id.get());
        }
    };
}

#endif // GIRIGARA_CORE_DEPERSONALIZATION_H
