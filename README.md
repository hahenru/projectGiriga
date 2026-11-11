# Project Girigara!

---

> 如您所見... 他媽的閉嘴！ ...額，抱歉，這恐怕真的很吵。

這是一個使用現代 C++ 自製的 2D 遊戲引擎與執行時期環境（Runtime）。

我將按常理為您提供一個視窗和一些場景，但目前仍然沒有場景編輯器供您使用。

此外，我們使用的腳本語言並非常見的 C# 或是 JavaScript。是的，我很抱歉，這兩個語言對我來說太複雜了。
*（笑點解析：你使用的可是全世界最複雜的語言 C++ 來作為引擎的基礎語言，外加 Lua 來寫腳本。）*

---

## 核心特性

- **現代 C++ 打造**：採用 C++20/C++23/C++26 標準、GLFW、GLAD 與 OpenGL 4.6 Core Profile。
- **純粹的自製 UI 系統**：
  - 支援相對父節點的局部座標階層（Parent-Child 座標樹）。
  - 支援百分比定位（支援精確至 0.01% 的浮動配置）與像素模式無縫切換。
  - 具備文字超界自適應縮放、動態字型貼圖烘焙（FreeType）。
  - 視窗拖曳具備邊界限制約束，畫面放大縮小自動校正防跑版。
- **內建除錯 Console 與 Shader 背景**：
  - In-Game Console 即時 REPL，直接調用 Lua 虛擬機全域環境與 C++ 註冊指令。
  - 後端 Shader 動態線性漸層與半透明高光泡泡效果。
- **熱插拔腳本架構**：
  - 啟動時自動掃描並常駐 `./assets/sysScripts/` 下的所有 Lua 系統腳本。
  - 原生 C++ 場景（`Scene`）與動態腳本場景（`LuaScene`）雙軌切換架構。

---

## 未來展望（畫餅階段）

未來我有機會引用最有名的 **ImGui** 作為引擎編輯器的部分。

不過到了那個時候，我得把 Engine 和 Runtime 徹底分開，這恐怕會很麻煩……所以就如上述，是**「有機會」**而非保證。

以下是近期目標:
加入sqlite的支援還有某種zip的解決方案，和一些額外的添加物，為了增添風味。
圖片載入的功能，但不是libpng、libjpg，他不夠全能。
glm或是其他數學庫

以下是遠期目標:
音訊。
https和websocket支持。
Engine, Runtime separate.

---

## 建置方式

開發環境採用 Fedora Linux，同時完全不保證Windows可以編譯成功，除非你願意自己修改Winodws的那部份makefile。
專案採用 Makefile 管理編譯，支援 GCC CLI 工具鏈：

```bash
# 編譯並建置專案
make linux

# 執行
./build/Girigara
```

一點額外事項，在這個版本中，我修改了makefile，所以我想之後你應該可以直接在windows編譯了，不過依賴的部分你得自己編譯，然後放到環境變數中CPATH和LIBRARY_PATH所在的位置。
或是我想你的包管理器可以替你完成這件事情，以下是使用的host是Windows 11 + mingw-w64

```bash
# 編譯並建置專案
make

# 執行
.\build\Girigara
```

對，就這麼簡單，我做了點判斷，現在他會看你的電腦是Windows嗎?但我沒考慮過都不是的部分。還請見諒。

## 系統依賴與軟體包需求

本專案採用 **C++23** 標準開發，建置前請確保系統環境已安裝相應的編譯器工具鏈與開發套件（Header / Development Libraries）。

### 依賴組件清單

| 組件名稱 | 建議版本 | 說明 | 鏈結參數 |
| :--- | :--- | :--- | :--- |
| **GCC / G++** | 13+ (支援 C++23) | 核心編譯工具鏈 | `-std=c++23` |
| **OpenGL** | 4.6 Core Profile | 核心圖形渲染 API | `-lGL` / `-lopengl32` |
| **GLFW** | 3.5.1 | 視窗建立、輸入事件監聽與 Context 管理 | `-lglfw` / `-lglfw3` |
| **FreeType 2** | 2.15.0 | 向量字型載入與動態點陣圖烘焙 | `pkg-config --libs freetype2` / `-lfreetype` |
| **Lua** | 5.4 | 腳本引擎與 In-Game REPL 執行環境 | `-llua` |
| **ALSA** *(Linux 專用)* | 最新版 | 底層音訊後端支援 | `-lasound` |
| **miniaudio** | 0.11.26 | 單一標頭檔音訊引擎（已內建實作包裝於專案中） | 需放置於 include 路徑 |
| **GLAD** | 4.6 Core | OpenGL Function Loader | 原始碼已內建於 `src/glad.c` |

以上依賴，除了glad以外，全部建議預編譯好，放在系統資料夾等待連結。
