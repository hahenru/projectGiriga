#include <Resource/AudioResource.h>
#include <fstream>
#include <iostream>
#include <algorithm>

AudioResource::AudioResource(std::string resolvedPath)
    : m_path(std::move(resolvedPath)) {
    detectFormat();
    m_isLoaded = loadFromFile();
}

void AudioResource::detectFormat() {
    std::string ext;
    auto dotPos = m_path.find_last_of('.');
    if (dotPos != std::string::npos) {
        ext = m_path.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    if (ext == "wav") {
        m_format = AudioFormat::WAV;
    } else if (ext == "ogg") {
        m_format = AudioFormat::OGG;
    } else if (ext == "mp3") {
        m_format = AudioFormat::MP3;
    } else {
        m_format = AudioFormat::Unknown;
    }
}

bool AudioResource::loadFromFile() {
    std::ifstream file(m_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[AudioResource] Failed to open audio file: " << m_path << "\n";
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    m_data.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(m_data.data()), size)) {
        std::cerr << "[AudioResource] Failed to read audio bytes from: " << m_path << "\n";
        m_data.clear();
        return false;
    }

    // 基礎 WAV 格式標頭檢驗與參數解析（簡易內建剖析器）
    if (m_format == AudioFormat::WAV && m_data.size() >= 44) {
        // "RIFF" 標籤
        if (m_data[0] == 'R' && m_data[1] == 'I' && m_data[2] == 'F' && m_data[3] == 'F') {
            m_channels = *reinterpret_cast<const uint16_t*>(&m_data[22]);
            m_sampleRate = *reinterpret_cast<const uint32_t*>(&m_data[24]);
            m_bitsPerSample = *reinterpret_cast<const uint16_t*>(&m_data[34]);
        }
    }

    return true;
}

bool AudioResource::reload() {
    m_data.clear();
    m_isLoaded = loadFromFile();
    return m_isLoaded;
}