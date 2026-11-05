#include <Audio/AudioSystem.h>
#include <iostream>

AudioSystem::AudioSystem() {
    // 1. 初始化音訊引擎（底層會自動起一個後台音訊線程）
    ma_result result = ma_engine_init(NULL, &m_engine);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioSystem] 初始化音訊引擎失敗！\n";
    }
}

AudioSystem::~AudioSystem() {
    stopBGM();
    // 釋放引擎資源
    ma_engine_uninit(&m_engine);
}

void AudioSystem::playBGM(const std::string& filePath, bool loop) {
    // 如果目前已經在播同一首，直接返回不中斷
    if (m_currentBGMPath == filePath && m_bgmLoaded) {
        return;
    }

    stopBGM();

    // 載入並初始化 BGM
    ma_result result = ma_sound_init_from_file(
        &m_engine,
        filePath.c_str(),
        MA_SOUND_FLAG_STREAM, // ★ BGM 使用串流讀取，不吃爆記憶體
        NULL,
        NULL,
        &m_bgm
    );

    if (result != MA_SUCCESS) {
        std::cerr << "[AudioSystem] 無法載入音訊檔案: " << filePath << "\n";
        return;
    }

    ma_sound_set_looping(&m_bgm, loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(&m_bgm);

    m_bgmLoaded = true;
    m_currentBGMPath = filePath;
}

void AudioSystem::stopBGM() {
    if (m_bgmLoaded) {
        ma_sound_stop(&m_bgm);
        ma_sound_uninit(&m_bgm);
        m_bgmLoaded = false;
        m_currentBGMPath.clear();
    }
}

void AudioSystem::playSFX(const std::string& filePath) {
    // 播放短音效（Fire-and-forget，底層播完自動回收）
    ma_engine_play_sound(&m_engine, filePath.c_str(), NULL);
}