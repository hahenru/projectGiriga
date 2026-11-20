# ProjectGirigara — 專案架構與開發脈絡
- **專案目標**
  - **概要** : 實作一套具備「顯示與邏輯解耦」、「多實體原生輸入（Multi-Seat）」與「多場景並行模擬」的雙端（Client / Headless Server）遊戲引擎（Decoupled Multi-Scene & Multi-Input Game Engine）。
  - **介紹** : 本專案打破傳統遊戲引擎「單一執行個體僅綁定單一輸入焦點與單一主場景」的限制，建立顯示（Presentation）與邏輯模擬（Simulation）完全分離的多對多架構。透過攔截並區分作業系統底層的多組實體鍵盤與滑鼠訊號（Raw Input / Multi-device），引擎允許單一主機同時獨立操控多個角色，無需啟動多個遊戲視窗即可提供原生的本機多人（Local Co-op / Multi-Seat）與連線混合體驗。
  - **核心特色** : 
    - **多場景並行與雙端同構（Multi-Scene Concurrency）**: 特殊的場景管理器（`SceneManager`），讓客戶端如同伺服器端一樣，可以同時持有多個場景的實體進行計算；支援主執行緒（`MainThread`）、執行緒池（`ThreadPoolTask`）與獨立專屬執行緒（`DedicatedThread`）三種步進模式，並可透過 `UIViewport` 將任意背景掛載場景離屏渲染嵌入引擎視窗。
    - **局部時間軸與獨立步進（Decoupled Local Delta-Time）**: 摒棄傳統引擎依賴全域單一 `dt`（Global Delta-Time）的更新模式，將時間步進下放至各別場景與局部空間（`SceneClock`）。每個場景皆擁有獨立的時間縮放（Time Scale）、局部群組時間倍率與累加器，從底層架構上解決了「多人連線與多場景並存下，局部時間暫停（Time-Stop）或慢動作技能會引發全域時間悖論」的難題。

## 1. 嚴格 Coding Style 與專案紅線（必須無條件遵守）
- **標頭與實作分離**：除模板與極短內聯存取器外，盡可能將宣告放在 `include/`（`.h`）、實作分離至 `src/`（`.cpp`），減少編譯耦合。
- **傳統 Include Guard**：所有 `.h` 標頭檔**必須**使用傳統的 `#ifndef GIRIGARA_MODULE_FILENAME_H` / `#define` / `#endif`（**禁用** `#pragma once`）。
- **事件驅動優先 (`EventBus`)**：子系統之間能走事件系統就盡量透過 `EventBus` 發布/訂閱事件解耦，避免直接跨模組持有指標或強耦合；物件銷毀時使用 `Identifiable` / `Depersonalization` 搭配 `unsubscribeByOwner(getID())` 退訂。
- **資料與邏輯分離**：資料結構（如設定表、UI 屬性表 `UIPropMap`、事件結構體）與業務邏輯處理分離，避免把狀態與複雜邏輯混寫在單一巨型類別中。
- **現代 C++23 與 RAII**：
  - 使用 **C++23 或以上標準**（`-std=c++23`）。
  - **少用裸指標**（僅在非擁有權的觀察者或 C API / Lua `lightuserdata` 邊界使用），資源與生命週期一律使用 RAII（`std::unique_ptr`、`std::shared_ptr`、自訂 RAII Guard 如 `GlfwContext` / `VulkanContext` / `VulkanTexture` / `VulkanRenderTarget` / `ENetContext`）。
- **專案目錄絕不放第三方依賴**：`include/` 與 `src/` 僅存放專案自身程式碼，所有第三方函式庫（Vulkan SDK、VMA、vk-bootstrap、GLM、GLFW、FreeType2、miniaudio、miniz、ENet、Lua 等）皆由系統套件或外部 sysroot（如 `/usr/local` 或 `C:\Users\Hartitska\sysRoot`）提供。

## 2. 技術棧與建置規範
- **語言與腳本**：C++23 + 原生 Lua 5.x C API（不使用 sol2 / LuaBridge 等第三方 C++ 綁定庫）。
- **圖形與渲染架構（純 Vulkan 1.3，已完全移除 OpenGL / GLAD）**：
  - **核心層**：`GlfwContext`（`GLFW_NO_API`）+ `VulkanContext`（透過 `vk-bootstrap` 初始化 Vulkan 1.3，啟用 `dynamicRendering` 與 `synchronization2`，採 `defer_surface_initialization()` 支援多視窗，Present Queue 共用 Graphics Queue）+ **VMA**（`vk_mem_alloc.h`）管理顯存緩衝區與貼圖。
  - **數學庫**：**GLM**（強制定義 `GLM_FORCE_RADIANS` 與 `GLM_FORCE_DEPTH_ZERO_TO_ONE`，對齊 Vulkan 左上原點與 `[0, 1]` 深度範圍）。
  - **著色器管線**：純 Vulkan GLSL 450 + 48-byte `UIPushConstants` + Combined Image Sampler，透過 `glslc` 編譯為 SPIR-V（`assets/shaders/*.spv`）；舊 OpenGL `Shader` 類別已完全移除。
  - **雙階段渲染與離屏視口**：`WindowManager` 管理獨立 `VkSurfaceKHR` 與 `VkSwapchainKHR`，主迴圈分為 **Pass 1**（`beginCommandBuffer` 後由 `EngineScene` 呼叫各 `UIViewport::renderSceneOffscreen` 渲染至 `VulkanRenderTarget`）與 **Pass 2**（`beginSwapchainRendering` 後由 `UIManager::render` 合成所有 `UIWindow` 與全螢幕 UI，最後 `endFrame` 提交呈現）。
  - **文字與圖片懶上傳**：`TextTexture` 僅在 CPU 端烘焙單通道點陣與維護 `m_revision`，由 `UIRenderer` 依 `revision` 自動懶上傳並快取至 GPU `VulkanTexture`。
- **建置系統**：純 GNU `Makefile`（不使用 CMake），`include/` 與 `src/` 嚴格 1:1 目錄對齊。
- **分流建置目標**：
  - `projectGIRIGARA`：預設整合版 Client（含本機 Server 能力）。
  - `Pure Client`：僅連線遠端，無 Server 模組。
  - `projectGIRIGARA_server`：無頭伺服器（定義 `GIRIGARA_SERVER`，零視窗、零音訊、零 Vulkan、零 `SettingsRegistry` 依賴，讀取自動生成的 `server.cfg`）。
  - `projectGIRIGARA_engine`：獨立遊戲引擎編輯器工作區入口（`src/engine_main.cpp` + `EngineScene`），支援在 `UIWindow` 中內嵌即時離屏渲染的 `UIViewport` 視口與 `~` 鍵喚出 `UIConsole`。
- **開發與發版環境**：Fedora Linux (KDE Plasma on Wayland, HiDPI) 與 Windows 原生 MinGW-w64，支援 Linux 端交叉編譯 Windows 版與 Flatpak (`com.hahenru.girigara`) / RPM 打包。
- **專案架構設計** : 絕不使用 ECS 架構，盡可能使用傳統 OOP。

## 3. 現有核心系統與已暴露的 Lua API
- **Lua 綁定機制**：子系統建構時發布 `RegisterScriptBindingEvent`（存入 `LUA_REGISTRYINDEX` 並註冊全域 `lua_CFunction`），解構時發布 `UnregisterScriptBindingEvent`；`LuaRef` 已封裝 `lua_State*`，無參數呼叫直接 `ref.push()`。
- **UI 系統 (`UIManager` + `UIFactory`)**：
  - 支援像素與百分比定位（`xPercent`, `yPercent`, `wPercent`, `hPercent`）以適應 HiDPI 與視窗縮放。
  - 內建元件：`Panel`、`Text`（UTF-8/UTF-32 轉碼）、`Button`（含冷卻與鎖定狀態）、`InputField`、`Window`（可拖曳獨立視口容器）、`Viewport`（`UIViewport` 離屏渲染畫布）、`VerticalLayout`、`HorizontalLayout`、`SlantedLayout`、`ScrollView`、`UIConsole`（動態氣泡 `console_bg.frag.spv`）、`UISettingsMenu`（由 `SettingsRegistry` 驅動）。
  - 已註冊 Lua API：`CreateUI`、`FindUI`、`GetUIByUUID`、`BindUIUpdate`、`UI_OnClick`、`UI_GetText`、`UI_SetText`、`UI_SetPosition`、`SetPositionPercent`、`UI_SetVisible`、`UI_SetButtonLocked`、`UI_SetFontSize`、`UI_GetScreenSize`。
- **更多詳細 Lua 綁定函數**
  - 上述並沒有完整陳述全部的可用函數，必要時候應該詢問使用 `make stub` 生成一個 Lua API Map。
  - 如果可以，盡可能引導將 Binder 函數寫到 `ModuleNameBinder.cpp`，並存放在 `./src/Binder/` 底下。

## 4. 協作輸出規範
- 修改既有檔案時請直接提供精準的局部修改區塊或完整函式，避免每次貼出整份數百行無關程式碼，並嚴格對齊上述架構與命名風格。
- 如果有需要特定功能或是函數，請不要瞎猜，應當請螢幕前的人類使用 `make map` 產生一個完整的 C++ 函數簽名地圖，內涵函數簽名和所在位置以及每一個檔案的依賴。
