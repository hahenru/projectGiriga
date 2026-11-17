# ProjectGirigara — 專案架構與開發脈絡

## 1. 嚴格 Coding Style 與專案紅線（必須無條件遵守）
- **標頭與實作分離**：除模板與極短內聯存取器外，盡可能將宣告放在 `include/`（`.h`）、實作分離至 `src/`（`.cpp`），減少編譯耦合。
- **傳統 Include Guard**：所有 `.h` 標頭檔**必須**使用傳統的 `#ifndef GIRIGARA_MODULE_FILENAME_H` / `#define` / `#endif`（**禁用** `#pragma once`）。
- **事件驅動優先 (`EventBus`)**：子系統之間能走事件系統就盡量透過 `EventBus` 發布/訂閱事件解耦，避免直接跨模組持有指標或強耦合；物件銷毀時使用 `Identifiable` / `Depersonalization` 搭配 `unsubscribeByOwner(getID())` 退訂。
- **資料與邏輯分離**：資料結構（如設定表、UI 屬性表 `UIPropMap`、事件結構體）與業務邏輯處理分離，避免把狀態與複雜邏輯混寫在單一巨型類別中。
- **現代 C++23 與 RAII**：
  - 使用 **C++23 或以上標準**（`-std=c++23`）。
  - **少用裸指標**（僅在非擁有權的觀察者或 C API / Lua `lightuserdata` 邊界使用），資源與生命週期一律使用 RAII（`std::unique_ptr`、`std::shared_ptr`、自訂 RAII Guard 如 `GlfwContext` / `ENetContext`）。
- **專案目錄絕不放第三方依賴**：`include/` 與 `src/` 僅存放專案自身程式碼，所有第三方函式庫（GLFW、FreeType2、miniaudio、miniz、ENet、Lua 等）皆由系統套件或外部 sysroot 提供。

## 2. 技術棧與建置規範
- **語言與腳本**：C++23 + 原生 Lua 5.x C API（不使用 sol2 / LuaBridge 等第三方 C++ 綁定庫）。
- **建置系統**：純 GNU `Makefile`（不使用 CMake），`include/` 與 `src/` 嚴格 1:1 目錄對齊。
- **四端分流建置目標**：
  - `projectGIRIGARA`：預設整合版 Client（含本機 Server 能力）。
  - `Pure Client`：僅連線遠端，無 Server 模組。
  - `projectGIRIGARA_server`：無頭伺服器（定義 `GIRIGARA_SERVER`，零視窗、零音訊、零 OpenGL 依賴，讀取自動生成的 `server.cfg`）。
  - `projectGIRIGARA_tool`：獨立 GUI 資源封包工具（零網路、零音訊依賴，載入 `assets/scripts/packer_tool.lua`）。
- **開發與發版環境**：Fedora Linux (KDE Plasma on Wayland, HiDPI)，支援 MinGW-w64 交叉編譯 Windows 版與 Flatpak (`com.hahenru.girigara`) 打包。

## 3. 現有核心系統與已暴露的 Lua API
- **Lua 綁定機制**：子系統建構時發布 `RegisterScriptBindingEvent`（存入 `LUA_REGISTRYINDEX` 並註冊全域 `lua_CFunction`），解構時發布 `UnregisterScriptBindingEvent`；`LuaRef` 已封裝 `lua_State*`，無參數呼叫直接 `ref.push()`。
- **UI 系統 (`UIManager` + `UIFactory`)**：
  - 支援像素與百分比定位（`xPercent`, `yPercent`, `wPercent`, `hPercent`）以適應 HiDPI 與視窗縮放。
  - 內建元件：`Panel`、`Text`（UTF-8/UTF-32 轉碼）、`Button`（含冷卻與鎖定狀態）、`InputField`、`Window`、`VerticalLayout`、`HorizontalLayout`、`SlantedLayout`、`ScrollView`、`UIConsole`、`UISettingsMenu`。
  - 已註冊 Lua API：`CreateUI`、`FindUI`、`GetUIByUUID`、`BindUIUpdate`、`UI_OnClick`、`UI_GetText`、`UI_SetText`、`UI_SetPosition`、`SetPositionPercent`、`UI_SetVisible`、`UI_SetButtonLocked`、`UI_SetFontSize`、`UI_GetScreenSize`。
- **資源封包 (`GpkArchive`)**：
  - 16-byte `GRGZ` 容器檔頭 + `miniz` 壓縮 + 隨機偏移 256-bit ChaCha20 串流加解密。
  - 封包類型：`Image (.imz)`、`Script (.luz)`、`Database (.dtz)`、`Audio (.auz)`。
  - 已註冊 Lua API：`GpkPackDirectory`、`GpkPackFiles`、`GpkList`、`GpkExtractAll`、`OpenFolderDialog`、`OpenFileDialog`，以及視窗拖放全域回呼 `OnFileDrop(paths)`。
- **更多詳細lua綁定函數**
  - 上述並沒有完整陳述全部的可用函數，必要時候應該詢問使用`make stub`生成一個lua api map。
  - 如果可以，盡可能引導將Binder函數寫到ModuleNameBinder.cpp，並存放在./scr/Binder/底下
 
## 4. 協作輸出規範
- 修改既有檔案時請直接提供精準的局部修改區塊或完整函式，避免每次貼出整份數百行無關程式碼，並嚴格對齊上述架構與命名風格。
- 如果有需要特定功能或是函數，請不要瞎猜，應當請螢幕前的人類使用`make map`產生一個完整的C++函數簽名地圖，內涵函數簽名和所在位置以及每一個檔案的依賴。