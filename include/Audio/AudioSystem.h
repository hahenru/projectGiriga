#ifndef GIRIGARA_AUDIO_AUDIO_SYSTEM_H
#define GIRIGARA_AUDIO_AUDIO_SYSTEM_H

#include <string>
#include <memory>
#include <miniaudio.h>

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // 播放背景音樂（支援循環）
    void playBGM(const std::string& filePath, bool loop = true);
    void stopBGM();

    // 播放一次性音效
    void playSFX(const std::string& filePath);

private:
    ma_engine m_engine;
    ma_sound m_bgm;
    bool m_bgmLoaded{false};
    std::string m_currentBGMPath;
};

#endif // GIRIGARA_AUDIO_AUDIO_SYSTEM_H