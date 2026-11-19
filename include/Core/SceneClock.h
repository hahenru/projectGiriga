#ifndef GIRIGARA_CORE_SCENE_CLOCK_H
#define GIRIGARA_CORE_SCENE_CLOCK_H

#include <cstdint>
#include <unordered_map>

struct SceneClock {
    float rawDt = 0.0f;             // 未縮放的真實幀間隔（UI、網路同步、時停不受影響特效用）
    float sceneDt = 0.0f;           // 經過場景整體倍率縮放後的時間（rawDt * sceneTimeScale）
    double totalRawTime = 0.0;      // 場景啟動以來的累積真實時間
    double totalSceneTime = 0.0;    // 場景累積的邏輯時間（時停期間不會增加）
    uint64_t tickCount = 0;         // 邏輯步數計數器

    float sceneTimeScale = 1.0f;    // 場景全域時間倍率（1.0 = 正常, 0.0 = 全圖時停）
    bool isPaused = false;          // 系統級暫停（如單機打開系統選單）

    // 支援「時間群組（Time Channel / Group）」，例如：0=預設, 1=時停結界A, 2=加速領域
    std::unordered_map<uint32_t, float> groupScales;

    // 計算特定群組或個體最終的有效 dt
    float getEffectiveDt(uint32_t groupId = 0, float entityScale = 1.0f) const {
        if (isPaused) return 0.0f;
        float grpScale = 1.0f;
        if (groupId != 0) {
            if (auto it = groupScales.find(groupId); it != groupScales.end()) {
                grpScale = it->second;
            }
        }
        return sceneDt * grpScale * entityScale;
    }

    void advance(float incomingRawDt) {
        rawDt = incomingRawDt;
        totalRawTime += static_cast<double>(incomingRawDt);
        if (isPaused) {
            sceneDt = 0.0f;
            return;
        }
        sceneDt = incomingRawDt * sceneTimeScale;
        totalSceneTime += static_cast<double>(sceneDt);
        ++tickCount;
    }
};

#endif // GIRIGARA_CORE_SCENE_CLOCK_H