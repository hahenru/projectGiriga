#ifndef GIRIGARA_RESOURCE_IMAGE_RESOURCE_H
#define GIRIGARA_RESOURCE_IMAGE_RESOURCE_H

#include <string>
#include <vector>
#include <cstdint>
#include <atomic>
#include <Resource/IResource.h>

class ImageResource : public IResource {
public:
    explicit ImageResource(std::string resolvedPath, bool flipVertically = true);
    ImageResource(std::string virtualPath, const std::vector<uint8_t>& encodedBytes, bool flipVertically = true);
    ~ImageResource() override = default;

    ImageResource(const ImageResource&) = delete;
    ImageResource& operator=(const ImageResource&) = delete;
    ImageResource(ImageResource&& other) noexcept;
    ImageResource& operator=(ImageResource&& other) noexcept;

    const std::string& getPath() const override { return m_path; }
    bool reload() override;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return m_channels; }
    const std::vector<uint8_t>& getPixels() const { return m_pixels; }

    bool isLoaded() const { return m_isReady.load(std::memory_order_acquire) && !m_pixels.empty(); }
    uint64_t getRevision() const { return m_revision.load(std::memory_order_acquire); }

private:
    bool loadFromFile();
    bool decodeFromMemory(const uint8_t* data, size_t size);

    std::string m_path;
    bool m_flipVertically{true};
    bool m_fromMemory{false};

    int m_width{0};
    int m_height{0};
    int m_channels{0};
    std::vector<uint8_t> m_pixels;

    std::atomic<bool> m_isReady{false};
    std::atomic<uint64_t> m_revision{0};
};

#endif // GIRIGARA_RESOURCE_IMAGE_RESOURCE_H
