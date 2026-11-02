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


