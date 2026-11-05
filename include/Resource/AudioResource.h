#ifndef GIRIGARA_RESOURCE_AUDIO_RESOURCE_H
#define GIRIGARA_RESOURCE_AUDIO_RESOURCE_H

#include <string>
#include <vector>
#include <cstdint>

enum class AudioFormat {
    Unknown,
    WAV,
    OGG,
    MP3
};

class AudioResource {
public:
    AudioResource(std::string resolvedPath);
    ~AudioResource() = default;

    AudioResource(const AudioResource&) = delete;
    AudioResource& operator=(const AudioResource&) = delete;
    AudioResource(AudioResource&&) noexcept = default;
    AudioResource& operator=(AudioResource&&) noexcept = default;

    const std::string& getPath() const { return m_path; }
    const std::vector<uint8_t>& getRawData() const { return m_data; }
    
    uint32_t getSampleRate() const { return m_sampleRate; }
    uint16_t getChannels() const { return m_channels; }
    uint16_t getBitsPerSample() const { return m_bitsPerSample; }
    AudioFormat getFormat() const { return m_format; }
    bool isLoaded() const { return m_isLoaded; }

    bool reload();

private:
    bool loadFromFile();
    void detectFormat();

    std::string m_path;
    std::vector<uint8_t> m_data;
    uint32_t m_sampleRate{44100};
    uint16_t m_channels{2};
    uint16_t m_bitsPerSample{16};
    AudioFormat m_format{AudioFormat::Unknown};
    bool m_isLoaded{false};
};

#endif // GIRIGARA_RESOURCE_AUDIO_RESOURCE_H