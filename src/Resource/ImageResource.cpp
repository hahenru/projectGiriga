#include <Resource/ImageResource.h>
#include <stb/stb_image.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <utility>

namespace {
    // CPU 端安全上限，防止惡意標頭導致記憶體爆炸；具體 GPU 上限由渲染層上傳時檢查
    constexpr int MAX_IMAGE_DIMENSION = 16384;
}

ImageResource::ImageResource(std::string resolvedPath, bool flipVertically)
: m_path(std::move(resolvedPath)),
m_flipVertically(flipVertically),
m_fromMemory(false) {
    loadFromFile();
}

ImageResource::ImageResource(std::string virtualPath, const std::vector<uint8_t>& encodedBytes, bool flipVertically)
: m_path(std::move(virtualPath)),
m_flipVertically(flipVertically),
m_fromMemory(true) {
    if (!encodedBytes.empty()) {
        decodeFromMemory(encodedBytes.data(), encodedBytes.size());
    }
}

ImageResource::ImageResource(ImageResource&& other) noexcept
: m_path(std::move(other.m_path)),
m_flipVertically(other.m_flipVertically),
m_fromMemory(other.m_fromMemory),
m_width(std::exchange(other.m_width, 0)),
m_height(std::exchange(other.m_height, 0)),
m_channels(std::exchange(other.m_channels, 0)),
m_pixels(std::move(other.m_pixels)),
m_isReady(other.m_isReady.load(std::memory_order_acquire)),
m_revision(other.m_revision.load(std::memory_order_acquire)) {
    other.m_isReady.store(false, std::memory_order_release);
}

ImageResource& ImageResource::operator=(ImageResource&& other) noexcept {
    if (this != &other) {
        m_path = std::move(other.m_path);
        m_flipVertically = other.m_flipVertically;
        m_fromMemory = other.m_fromMemory;
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_channels = std::exchange(other.m_channels, 0);
        m_pixels = std::move(other.m_pixels);
        m_isReady.store(other.m_isReady.load(std::memory_order_acquire), std::memory_order_release);
        m_revision.store(other.m_revision.load(std::memory_order_acquire), std::memory_order_release);
        other.m_isReady.store(false, std::memory_order_release);
    }
    return *this;
}

bool ImageResource::loadFromFile() {
    std::ifstream file(m_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[ImageResource] Failed to open image file: " << m_path << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    if (size <= 0) {
        std::cerr << "[ImageResource] Empty image file: " << m_path << "\n";
        return false;
    }

    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        std::cerr << "[ImageResource] Failed to read bytes from: " << m_path << "\n";
        return false;
    }

    return decodeFromMemory(buffer.data(), buffer.size());
}

bool ImageResource::decodeFromMemory(const uint8_t* data, size_t size) {
    if (!data || size == 0) return false;

    int infoW = 0, infoH = 0, infoComp = 0;
    if (!stbi_info_from_memory(data, static_cast<int>(size), &infoW, &infoH, &infoComp)) {
        std::cerr << "[ImageResource] Invalid or unsupported image header: " << m_path << "\n";
        return false;
    }

    if (infoW <= 0 || infoH <= 0 || infoW > MAX_IMAGE_DIMENSION || infoH > MAX_IMAGE_DIMENSION) {
        std::cerr << "[ImageResource] Image dimensions out of bounds ("
        << infoW << "x" << infoH << "): " << m_path << "\n";
        return false;
    }

    // 注意：stbi_set_flip_vertically_on_load 為全域狀態，若多執行緒同時使用不同 flip 設定需加鎖或統一翻轉策略
    stbi_set_flip_vertically_on_load(m_flipVertically ? 1 : 0);

    int width = 0, height = 0, origChannels = 0;
    unsigned char* rawPixels = stbi_load_from_memory(
        data, static_cast<int>(size),
                                                     &width, &height, &origChannels,
                                                     STBI_rgb_alpha
    );

    if (!rawPixels) {
        std::cerr << "[ImageResource] Failed to decode image (" << m_path << "): "
        << (stbi_failure_reason() ? stbi_failure_reason() : "Unknown") << "\n";
        return false;
    }

    const size_t byteCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    std::vector<uint8_t> decoded(byteCount);
    std::memcpy(decoded.data(), rawPixels, byteCount);
    stbi_image_free(rawPixels);

    m_isReady.store(false, std::memory_order_release);
    m_width = width;
    m_height = height;
    m_channels = origChannels;
    m_pixels = std::move(decoded);
    m_revision.fetch_add(1, std::memory_order_release);
    m_isReady.store(true, std::memory_order_release);

    return true;
}

bool ImageResource::reload() {
    if (m_fromMemory) {
        std::cerr << "[ImageResource] Cannot reload memory-backed image directly from disk: " << m_path << "\n";
        return false;
    }
    return loadFromFile();
}
