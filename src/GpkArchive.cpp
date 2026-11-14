#include <Resource/GpkArchive.h>
#include <cstring>
#include <random>
#include <iostream>
#include <algorithm>

namespace {
    constexpr uint8_t GPK_MAGIC[4] = { 'G', 'R', 'G', 'Z' };
    constexpr size_t GPK_HEADER_SIZE = 16;

    inline uint32_t rotl32(uint32_t v, int c) {
        return (v << c) | (v >> (32 - c));
    }

    inline void quarterRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl32(d, 16);
        c += d; b ^= c; b = rotl32(b, 12);
        a += b; d ^= a; d = rotl32(d, 8);
        c += d; b ^= c; b = rotl32(b, 7);
    }

    void chacha20Block(uint32_t out[16], const uint32_t keyWords[8], uint64_t counter, const uint32_t nonceWords[2]) {
        uint32_t x[16] = {
            0x61707865u, 0x3320646eu, 0x79622d32u, 0x6b206574u, // "expand 32-byte k"
            keyWords[0], keyWords[1], keyWords[2], keyWords[3],
            keyWords[4], keyWords[5], keyWords[6], keyWords[7],
            static_cast<uint32_t>(counter & 0xFFFFFFFFull),
            static_cast<uint32_t>((counter >> 32) & 0xFFFFFFFFull),
            nonceWords[0], nonceWords[1]
        };

        uint32_t w[16];
        std::memcpy(w, x, sizeof(x));

        for (int i = 0; i < 10; ++i) {
            quarterRound(w[0], w[4], w[8],  w[12]);
            quarterRound(w[1], w[5], w[9],  w[13]);
            quarterRound(w[2], w[6], w[10], w[14]);
            quarterRound(w[3], w[7], w[11], w[15]);
            quarterRound(w[0], w[5], w[10], w[15]);
            quarterRound(w[1], w[6], w[11], w[12]);
            quarterRound(w[2], w[7], w[8],  w[13]);
            quarterRound(w[3], w[4], w[9],  w[14]);
        }

        for (int i = 0; i < 16; ++i) {
            out[i] = w[i] + x[i];
        }
    }
}

void GpkArchive::chacha20Xor(uint8_t* buffer, size_t size, uint64_t streamOffset,
const Key256& key, const uint8_t nonce[8]) {
    if (!buffer || size == 0) return;

    uint32_t keyWords[8];
    uint32_t nonceWords[2];
    std::memcpy(keyWords, key.data(), 32);
    std::memcpy(nonceWords, nonce, 8);

    uint64_t blockIdx = streamOffset / 64;
    size_t byteInBlock = static_cast<size_t>(streamOffset % 64);

    uint32_t keystreamWords[16];
    uint8_t keystreamBytes[64];

    size_t processed = 0;
    while (processed < size) {
        chacha20Block(keystreamWords, keyWords, blockIdx, nonceWords);
        std::memcpy(keystreamBytes, keystreamWords, 64);

        size_t chunk = std::min<size_t>(64 - byteInBlock, size - processed);
        for (size_t i = 0; i < chunk; ++i) {
            buffer[processed + i] ^= keystreamBytes[byteInBlock + i];
        }

        processed += chunk;
        ++blockIdx;
        byteInBlock = 0;
    }
}

GpkArchive::~GpkArchive() {
    close();
}

void GpkArchive::close() {
    if (m_isOpen) {
        mz_zip_reader_end(&m_zip);
        m_isOpen = false;
    }
    if (m_file.is_open()) {
        m_file.close();
    }
    m_entryIndex.clear();
}

size_t GpkArchive::onMinizRead(void* pOpaque, mz_uint64 fileOfs, void* pBuf, size_t n) {
    auto* self = static_cast<GpkArchive*>(pOpaque);
    if (!self || !self->m_file.is_open() || n == 0) return 0;

    // 跳過前 16-byte 自訂容器檔頭
    self->m_file.clear();
    self->m_file.seekg(static_cast<std::streamoff>(GPK_HEADER_SIZE + fileOfs), std::ios::beg);
    if (!self->m_file.good()) return 0;

    self->m_file.read(reinterpret_cast<char*>(pBuf), static_cast<std::streamsize>(n));
    size_t bytesRead = static_cast<size_t>(self->m_file.gcount());

    if (bytesRead > 0 && self->m_encrypted) {
        chacha20Xor(static_cast<uint8_t*>(pBuf), bytesRead, fileOfs, self->m_key, self->m_nonce);
    }
    return bytesRead;
}

bool GpkArchive::open(const std::string& archivePath, const Key256& key) {
    close();

    m_file.open(archivePath, std::ios::binary | std::ios::ate);
    if (!m_file.is_open()) {
        std::cerr << "[GpkArchive] Cannot open archive: " << archivePath << "\n";
        return false;
    }

    std::streamsize totalSize = m_file.tellg();
    if (totalSize <= static_cast<std::streamsize>(GPK_HEADER_SIZE)) {
        std::cerr << "[GpkArchive] File too small: " << archivePath << "\n";
        close();
        return false;
    }

    m_file.seekg(0, std::ios::beg);
    uint8_t header[GPK_HEADER_SIZE]{};
    m_file.read(reinterpret_cast<char*>(header), GPK_HEADER_SIZE);

    if (std::memcmp(header, GPK_MAGIC, 4) != 0) {
        std::cerr << "[GpkArchive] Invalid container magic header: " << archivePath << "\n";
        close();
        return false;
    }

    m_type = static_cast<GpkType>(header[4]);
    m_encrypted = (header[6] & 0x01) != 0;
    m_key = key;
    std::memcpy(m_nonce, &header[8], 8);

    mz_zip_zero_struct(&m_zip);
    m_zip.m_pRead = &GpkArchive::onMinizRead;
    m_zip.m_pIO_opaque = this;

    const mz_uint64 payloadSize = static_cast<mz_uint64>(totalSize) - GPK_HEADER_SIZE;
    if (!mz_zip_reader_init(&m_zip, payloadSize, 0)) {
        std::cerr << "[GpkArchive] Failed to parse inner archive (wrong key or corrupted): " << archivePath << "\n";
        close();
        return false;
    }

    m_isOpen = true;
    const mz_uint numFiles = mz_zip_reader_get_num_files(&m_zip);
    for (mz_uint i = 0; i < numFiles; ++i) {
        if (mz_zip_reader_is_file_a_directory(&m_zip, i)) continue;
        char filename[512]{};
        mz_zip_reader_get_filename(&m_zip, i, filename, sizeof(filename));
        m_entryIndex.emplace(filename, i);
    }

    return true;
}

bool GpkArchive::hasFile(const std::string& virtualPath) const {
    return m_entryIndex.find(virtualPath) != m_entryIndex.end();
}

std::vector<std::string> GpkArchive::listFiles() const {
    std::vector<std::string> result;
    result.reserve(m_entryIndex.size());
    for (const auto& [name, _] : m_entryIndex) {
        result.push_back(name);
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool GpkArchive::extractFile(const std::string& virtualPath, std::vector<uint8_t>& outData) const {
    if (!m_isOpen) return false;

    auto it = m_entryIndex.find(virtualPath);
    if (it == m_entryIndex.end()) return false;

    mz_zip_archive_file_stat stat{};
    if (!mz_zip_reader_file_stat(&m_zip, it->second, &stat)) return false;

    outData.resize(static_cast<size_t>(stat.m_uncomp_size));
    if (outData.empty()) return true;

    if (!mz_zip_reader_extract_to_mem(&m_zip, it->second, outData.data(), outData.size(), 0)) {
        outData.clear();
        return false;
    }
    return true;
}

bool GpkArchive::packFiles(const std::string& outputArchivePath,
                           GpkType type,
                           const std::vector<PackEntry>& entries,
                           const Key256& key,
                           bool encrypt) {
    mz_zip_archive writer{};
    mz_zip_zero_struct(&writer);

    if (!mz_zip_writer_init_heap(&writer, 0, 1024 * 64)) {
        return false;
    }

    for (const auto& entry : entries) {
        std::ifstream in(entry.sourceDiskPath, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            mz_zip_writer_end(&writer);
            return false;
        }
        std::streamsize sz = in.tellg();
        in.seekg(0, std::ios::beg);
        std::vector<uint8_t> raw(static_cast<size_t>(std::max<std::streamsize>(0, sz)));
        if (sz > 0) {
            in.read(reinterpret_cast<char*>(raw.data()), sz);
        }

        if (!mz_zip_writer_add_mem(&writer, entry.virtualPath.c_str(), raw.data(), raw.size(), MZ_BEST_COMPRESSION)) {
            mz_zip_writer_end(&writer);
            return false;
        }
    }

    void* pZipData = nullptr;
    size_t zipSize = 0;
    if (!mz_zip_writer_finalize_heap_archive(&writer, &pZipData, &zipSize)) {
        mz_zip_writer_end(&writer);
        return false;
    }

    std::vector<uint8_t> payload(static_cast<uint8_t*>(pZipData), static_cast<uint8_t*>(pZipData) + zipSize);
    mz_free(pZipData);
    mz_zip_writer_end(&writer);

    // 準備 16-byte 檔頭與隨機 Nonce
    uint8_t header[GPK_HEADER_SIZE]{};
    std::memcpy(header, GPK_MAGIC, 4);
    header[4] = static_cast<uint8_t>(type);
    header[5] = 0x01; // Version 1
    header[6] = encrypt ? 0x01 : 0x00;

    uint8_t nonce[8]{};
    std::random_device rd;
    for (int i = 0; i < 8; ++i) {
        nonce[i] = static_cast<uint8_t>(rd() & 0xFF);
    }
    std::memcpy(&header[8], nonce, 8);

    if (encrypt) {
        chacha20Xor(payload.data(), payload.size(), 0, key, nonce);
    }

    std::ofstream out(outputArchivePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<const char*>(header), GPK_HEADER_SIZE);
    out.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
    return out.good();
}
