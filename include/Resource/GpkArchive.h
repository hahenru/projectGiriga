#ifndef GIRIGARA_RESOURCE_GPK_ARCHIVE_H
#define GIRIGARA_RESOURCE_GPK_ARCHIVE_H

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <fstream>
#include <miniz/miniz.h>

enum class GpkType : uint8_t {
    Unknown  = 0x00,
    Image    = 0x01, // .imz
    Script   = 0x02, // .luz
    Database = 0x03, // .dtz
    Audio    = 0x04  // .auz
};

class GpkArchive {
public:
    using Key256 = std::array<uint8_t, 32>;

    GpkArchive() = default;
    ~GpkArchive();

    GpkArchive(const GpkArchive&) = delete;
    GpkArchive& operator=(const GpkArchive&) = delete;

    // --- 讀取端（遊戲引擎 & GUI 解包預覽共用）---
    bool open(const std::string& archivePath, const Key256& key);
    void close();

    bool isOpen() const { return m_isOpen; }
    GpkType getType() const { return m_type; }
    bool hasFile(const std::string& virtualPath) const;
    std::vector<std::string> listFiles() const;
    bool extractFile(const std::string& virtualPath, std::vector<uint8_t>& outData) const;

    // --- 寫入端（供 GUI 打包工具呼叫，將資料夾或檔案清單打包成加密 .imz/.luz/.dtz）---
    struct PackEntry {
        std::string virtualPath;   // 壓縮包內路徑，例如 "ui/btn.png"
        std::string sourceDiskPath; // 本機實體檔案路徑
    };

    static bool packFiles(const std::string& outputArchivePath,
                          GpkType type,
                          const std::vector<PackEntry>& entries,
                          const Key256& key,
                          bool encrypt = true);

    // 支援隨機偏移的 ChaCha20 串流加解密（加解密同一函式）
    static void chacha20Xor(uint8_t* buffer, size_t size, uint64_t streamOffset,
                            const Key256& key, const uint8_t nonce[8]);

private:
    static size_t onMinizRead(void* pOpaque, mz_uint64 fileOfs, void* pBuf, size_t n);

    mutable std::ifstream m_file;
    mutable mz_zip_archive m_zip{};
    std::unordered_map<std::string, mz_uint> m_entryIndex;

    Key256 m_key{};
    uint8_t m_nonce[8]{};
    GpkType m_type{GpkType::Unknown};
    bool m_encrypted{false};
    bool m_isOpen{false};
};

#endif // GIRIGARA_RESOURCE_GPK_ARCHIVE_H
