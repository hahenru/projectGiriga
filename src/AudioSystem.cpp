#include <Audio/AudioSystem.h>
#include <iostream>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
}

namespace {
    constexpr const char* AUDIO_REGISTRY_KEY = "GIRIGARA_AUDIO_INSTANCE";

    AudioSystem* getAudioFromLua(lua_State* L) {
        lua_pushstring(L, AUDIO_REGISTRY_KEY);
        lua_gettable(L, LUA_REGISTRYINDEX);
        auto* audio = static_cast<AudioSystem*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return audio;
    }

    int lua_Audio_PlayBGM(lua_State* L) {
        const char* path = luaL_checkstring(L, 1);
        bool loop = lua_isboolean(L, 2) ? (lua_toboolean(L, 2) != 0) : true;

        if (path) {
            if (AudioSystem* audio = getAudioFromLua(L)) {
                audio->playBGM(path, loop);
            }
        }
        return 0;
    }

    int lua_Audio_StopBGM(lua_State* L) {
        if (AudioSystem* audio = getAudioFromLua(L)) {
            audio->stopBGM();
        }
        return 0;
    }

    int lua_Audio_PlaySFX(lua_State* L) {
        const char* path = luaL_checkstring(L, 1);
        if (path) {
            if (AudioSystem* audio = getAudioFromLua(L)) {
                audio->playSFX(path);
            }
        }
        return 0;
    }
}

AudioSystem::AudioSystem(EventBus& eventBus)
: m_eventBus(eventBus) {
    // 1. 初始化音訊引擎（底層會自動起一個後台音訊線程）
    ma_result result = ma_engine_init(NULL, &m_engine);
    if (result != MA_SUCCESS) {
        std::cerr << "[AudioSystem] 初始化音訊引擎失敗！\n";
    }

    registerScriptBindings();
}

AudioSystem::~AudioSystem() {
    unregisterScriptBindings();
    stopBGM();
    // 釋放引擎資源
    ma_engine_uninit(&m_engine);
}

void AudioSystem::registerScriptBindings() {
    m_eventBus.publish(RegisterScriptBindingEvent{
        getID(),
        AUDIO_REGISTRY_KEY,
        this,
        {
            { "Audio_PlayBGM", lua_Audio_PlayBGM },
            { "Audio_StopBGM", lua_Audio_StopBGM },
            { "Audio_PlaySFX", lua_Audio_PlaySFX }
        }
    });
}

void AudioSystem::unregisterScriptBindings() {
    m_eventBus.publish(UnregisterScriptBindingEvent{
        getID()
    });
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
