# ==========================================
# 跨平台作業系統環境偵測與指令變數
# ==========================================
ifeq ($(OS),Windows_NT)
    HOST_OS      = Windows
    DEFAULT_GOAL = win64-native-all
    PYTHON       = py
    TREE_CMD     = tree . /f > tree.txt
else
    HOST_OS      = Linux
    DEFAULT_GOAL = all
    PYTHON       = python3
    TREE_CMD     = tree -a > tree.txt
endif

.DEFAULT_GOAL := $(DEFAULT_GOAL)

# ==========================================
# 原始碼分類 (嚴格依據目錄結構分流，1:1 對齊 include/)
# ==========================================
# 1. 三端共用核心 (Common - 零網路、零音訊、零視窗依賴)
COMMON_SRC = \
	src/Core/CoreContext.cpp \
	src/System/ThreadPool.cpp \
	src/System/LoggerSystem.cpp \
	src/System/ConsoleSystem.cpp \
	src/System/FileSystem.cpp \
	src/Scene/Scene.cpp \
	src/Scene/SceneManager.cpp \
	src/Scene/LuaScene.cpp \
	src/Script/ScriptEngine.cpp \
	src/Resource/ScriptResource.cpp \
	src/Resource/GpkArchive.cpp \
	src/System/SettingsRegistry.cpp \
	src/Girigara.cpp \
	src/main.cpp

# 2. 網路模組 (Net - 供 Client 用，Tool 不編譯)
NET_SRC = \
	src/Network/ENetNode.cpp

# 3. 圖形與介面共用層 (GUI Common - 供 Client 用，Server 不編譯)
GUI_COMMON_SRC = \
	src/glad/glad.c \
	src/Shader/Shader.cpp \
	src/Window/WindowManager.cpp \
	src/System/KeyboardSystem.cpp \
	src/Font/FontLoader.cpp \
	src/Resource/FontResource.cpp \
	src/Resource/ImageResource.cpp \
	src/ThirdParty/stb_image_impl.cpp \
	src/Renderer/QuadMesh.cpp \
	src/Renderer/UIRenderer.cpp \
	src/UI/UIElement.cpp \
	src/UI/UIContainer.cpp \
	src/UI/UIPanel.cpp \
	src/UI/UIButton.cpp \
	src/UI/UIText.cpp \
	src/UI/UIWindow.cpp \
	src/UI/UIHorizontalLayout.cpp \
	src/UI/UIVerticalLayout.cpp \
	src/UI/UISlantedLayout.cpp \
	src/UI/UIManager.cpp \
	src/UI/UIFactory.cpp

# 4. 客戶端專屬 (Client - 遊戲場景、音訊系統、全域設定表)
CLIENT_SRC = \
	$(GUI_COMMON_SRC) \
	src/Client/ClientResourceManager.cpp \
	src/Scene/MainMenuScene.cpp \
	src/Scene/GameScene.cpp \
	src/Audio/AudioSystem.cpp \
	src/Resource/AudioResource.cpp \
	src/ThirdParty/miniaudio_impl.cpp \
	src/UI/UISettingsMenu.cpp \
    src/UI/UIConsole.cpp




# 5. 伺服器專屬 (Server - 無頭伺服器與外部設定檔)
SERVER_SRC = \
	src/Server/ServerResourceManager.cpp \
	src/Server/ServerConfig.cpp

# 組合目標原始碼
ALL_CLIENT_SRC      = $(COMMON_SRC) $(NET_SRC) $(CLIENT_SRC) $(SERVER_SRC)
ALL_PURE_CLIENT_SRC = $(COMMON_SRC) $(NET_SRC) $(CLIENT_SRC)
ALL_SERVER_SRC      = $(COMMON_SRC) $(NET_SRC) $(SERVER_SRC)

# ==========================================
# 目錄結構與產出路徑集中定義 (三端完全對稱)
# ==========================================
ROOT_BUILD_DIR          = build
ASSETS_DIR              = assets

# --- [1] Linux 原生輸出路徑 (build/linux/) ---
LINUX_OUT_DIR           = $(ROOT_BUILD_DIR)/linux
LINUX_CLIENT_OBJ_DIR    = $(LINUX_OUT_DIR)/obj/Client
LINUX_SERVER_OBJ_DIR    = $(LINUX_OUT_DIR)/obj/Server

LINUX_TARGET            = $(LINUX_OUT_DIR)/projectGIRIGARA
LINUX_SERVER_TARGET     = $(LINUX_OUT_DIR)/projectGIRIGARA_server
PACKAGE_LINUX           = $(LINUX_OUT_DIR)/ProjectGirigara_linux.tar.gz

# --- [2] Windows 交叉編譯輸出路徑 (build/win64_cross/) ---
WIN_CROSS_OUT_DIR       = $(ROOT_BUILD_DIR)/win64_cross
WIN_CROSS_CLIENT_OBJ_DIR= $(WIN_CROSS_OUT_DIR)/obj/Client
WIN_CROSS_SERVER_OBJ_DIR= $(WIN_CROSS_OUT_DIR)/obj/Server

WIN_CROSS_TARGET        = $(WIN_CROSS_OUT_DIR)/projectGIRIGARA.exe
WIN_CROSS_SERVER_TARGET = $(WIN_CROSS_OUT_DIR)/projectGIRIGARA_server.exe
PACKAGE_WIN_CROSS       = $(WIN_CROSS_OUT_DIR)/ProjectGirigara_win64_cross.zip

# --- [3] Windows 原生編譯輸出路徑 (build/win64_native/) ---
WIN_NAT_OUT_DIR         = $(ROOT_BUILD_DIR)/win64_native
WIN_NAT_CLIENT_OBJ_DIR  = $(WIN_NAT_OUT_DIR)/obj/Client
WIN_NAT_SERVER_OBJ_DIR  = $(WIN_NAT_OUT_DIR)/obj/Server

WIN_NAT_TARGET          = $(WIN_NAT_OUT_DIR)/projectGIRIGARA.exe
WIN_NAT_SERVER_TARGET   = $(WIN_NAT_OUT_DIR)/projectGIRIGARA_server.exe
PACKAGE_WIN_NAT         = $(WIN_NAT_OUT_DIR)/ProjectGirigara_win64_native.zip

# ==========================================
# [1] 原生 Linux 工具鏈與旗標
# ==========================================
CXX = g++
CC  = gcc
CXXFLAGS = -g -std=c++23 -MMD -MP -fPIE -I./include $(shell pkg-config --cflags freetype2 miniz 2>/dev/null)
CFLAGS   = -g -MMD -MP -fPIE -I./include

LINUX_LDFLAGS        = -lglfw -lGL -ldl -llua -lpthread -lasound -lm -lenet $(shell pkg-config --libs freetype2 miniz 2>/dev/null)
LINUX_SERVER_LDFLAGS = -ldl -llua -lpthread -lm -lenet $(shell pkg-config --libs miniz 2>/dev/null)

# ==========================================
# [2] MinGW-w64 Windows 交叉編譯設定 (Linux 端執行)
# ==========================================
WIN_CROSS_WINDRES = x86_64-w64-mingw32-windres
WIN_CROSS_CXX     = x86_64-w64-mingw32-g++
WIN_CROSS_CC      = x86_64-w64-mingw32-gcc
WIN_CROSS_PREFIX  = /usr/x86_64-w64-mingw32/sys-root/mingw

WIN_CROSS_CXXFLAGS = -O2 -std=c++23 -MMD -MP -I./include -I$(WIN_CROSS_PREFIX)/include/freetype2
WIN_CROSS_CFLAGS   = -O2 -MMD -MP -I./include

WIN_CROSS_COMMON_LIBS = -L$(WIN_CROSS_PREFIX)/lib -static-libgcc -static-libstdc++ -static

WIN_CROSS_LDFLAGS = -mwindows -Wl,--subsystem,windows $(WIN_CROSS_COMMON_LIBS) \
                    -lglfw3 -llua -lopengl32 -lole32 -lfreetype -lpng -lminiz -lz -lbz2 \
                    -lgdi32 -limm32 -lpthread -lenet -lws2_32 -lwinmm

WIN_CROSS_SERVER_LDFLAGS = $(WIN_CROSS_COMMON_LIBS) \
                           -llua -lpthread -lenet -lws2_32 -lwinmm -lminiz -lz

# ==========================================
# [3] 原生 Windows (MinGW-w64) 編譯設定 (Windows 端執行)
# ==========================================
WIN_NAT_WINDRES = windres
WIN_NAT_CXX     = g++
WIN_NAT_CC      = gcc
WIN_NAT_CXXFLAGS = -g -std=c++23 -MMD -MP -I./include
WIN_NAT_CFLAGS   = -g -MMD -MP -I./include

WIN_NAT_LDFLAGS = -mwindows -Wl,--subsystem,windows \
                  -lglfw3 -lopengl32 -llua -lfreetype -lgdi32 -lpthread -lole32 \
                  -lenet -lws2_32 -lwinmm -lminiz

WIN_NAT_SERVER_LDFLAGS = -llua -lpthread -lenet -lws2_32 -lwinmm -lminiz

# ==========================================
# 目標檔 (.o) 與依賴檔 (.d) 路徑映射
# ==========================================
# Linux
LINUX_CLIENT_OBJS = $(patsubst src/%.cpp, $(LINUX_CLIENT_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
                    $(patsubst src/%.c,   $(LINUX_CLIENT_OBJ_DIR)/%.o, $(filter %.c,   $(ALL_CLIENT_SRC)))
LINUX_SERVER_OBJS = $(patsubst src/%.cpp, $(LINUX_SERVER_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_SERVER_SRC)))

# Windows Cross
WIN_CROSS_CLIENT_OBJS = $(patsubst src/%.cpp, $(WIN_CROSS_CLIENT_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
                        $(patsubst src/%.c,   $(WIN_CROSS_CLIENT_OBJ_DIR)/%.o, $(filter %.c,   $(ALL_CLIENT_SRC)))
WIN_CROSS_SERVER_OBJS = $(patsubst src/%.cpp, $(WIN_CROSS_SERVER_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_SERVER_SRC)))

# Windows Native
WIN_NAT_CLIENT_OBJS = $(patsubst src/%.cpp, $(WIN_NAT_CLIENT_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
                      $(patsubst src/%.c,   $(WIN_NAT_CLIENT_OBJ_DIR)/%.o, $(filter %.c,   $(ALL_CLIENT_SRC)))
WIN_NAT_SERVER_OBJS = $(patsubst src/%.cpp, $(WIN_NAT_SERVER_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_SERVER_SRC)))

DEPS = $(LINUX_CLIENT_OBJS:.o=.d) $(LINUX_SERVER_OBJS:.o=.d) $(LINUX_TOOL_OBJS:.o=.d) \
       $(WIN_CROSS_CLIENT_OBJS:.o=.d) $(WIN_CROSS_SERVER_OBJS:.o=.d) \
       $(WIN_NAT_CLIENT_OBJS:.o=.d) $(WIN_NAT_SERVER_OBJS:.o=.d)

# ==========================================
# 總匯編譯入口
# ==========================================
all: linux-all win64-all flatpak

# ==========================================
# [1] Linux 原生建置流程 (build/linux/)
# ==========================================
linux-all: linux-client linux-server copy_assets_linux package_linux
linux: linux-client copy_assets_linux package_linux

linux-client: $(LINUX_TARGET)
$(LINUX_TARGET): $(LINUX_CLIENT_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(LINUX_CLIENT_OBJS) -o $@ $(LINUX_LDFLAGS)

$(LINUX_CLIENT_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LINUX_CLIENT_OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

linux-server: $(LINUX_SERVER_TARGET)
$(LINUX_SERVER_TARGET): $(LINUX_SERVER_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(LINUX_SERVER_OBJS) -o $@ $(LINUX_SERVER_LDFLAGS)

$(LINUX_SERVER_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DGIRIGARA_SERVER -c $< -o $@

copy_assets_linux:
	@mkdir -p $(LINUX_OUT_DIR)
	@rm -rf $(LINUX_OUT_DIR)/$(ASSETS_DIR)
	@cp -r $(ASSETS_DIR) $(LINUX_OUT_DIR)/

package_linux: $(LINUX_TARGET) $(LINUX_SERVER_TARGET) copy_assets_linux
	@echo "==> 打包 Linux 完整發布包至 $(PACKAGE_LINUX) ..."
	@tar -czf $(PACKAGE_LINUX) -C $(LINUX_OUT_DIR) \
		$(notdir $(LINUX_TARGET)) $(notdir $(LINUX_SERVER_TARGET)) $(ASSETS_DIR)

# ==========================================
# [2] Windows 交叉編譯流程 (build/win64_cross/)
# ==========================================
win64-all: win64-client win64-server copy_assets_win_cross package_win_cross
win64: win64-client copy_assets_win_cross package_win_cross

$(WIN_CROSS_CLIENT_OBJ_DIR)/GirigaraRC.o: Girigara.rc
	@mkdir -p $(dir $@)
	$(WIN_CROSS_WINDRES) $< -o $@

win64-client: $(WIN_CROSS_TARGET)
$(WIN_CROSS_TARGET): $(WIN_CROSS_CLIENT_OBJS) $(WIN_CROSS_CLIENT_OBJ_DIR)/GirigaraRC.o
	@mkdir -p $(dir $@)
	$(WIN_CROSS_CXX) $(WIN_CROSS_CLIENT_OBJS) $(WIN_CROSS_CLIENT_OBJ_DIR)/GirigaraRC.o -o $@ $(WIN_CROSS_LDFLAGS)

$(WIN_CROSS_CLIENT_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(WIN_CROSS_CXX) $(WIN_CROSS_CXXFLAGS) -c $< -o $@

$(WIN_CROSS_CLIENT_OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(WIN_CROSS_CC) $(WIN_CROSS_CFLAGS) -c $< -o $@

win64-server: $(WIN_CROSS_SERVER_TARGET)
$(WIN_CROSS_SERVER_TARGET): $(WIN_CROSS_SERVER_OBJS)
	@mkdir -p $(dir $@)
	$(WIN_CROSS_CXX) $(WIN_CROSS_SERVER_OBJS) -o $@ $(WIN_CROSS_SERVER_LDFLAGS)

$(WIN_CROSS_SERVER_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(WIN_CROSS_CXX) $(WIN_CROSS_CXXFLAGS) -DGIRIGARA_SERVER -c $< -o $@

copy_assets_win_cross:
	@mkdir -p $(WIN_CROSS_OUT_DIR)
	@rm -rf $(WIN_CROSS_OUT_DIR)/$(ASSETS_DIR)
	@cp -r $(ASSETS_DIR) $(WIN_CROSS_OUT_DIR)/

package_win_cross: $(WIN_CROSS_TARGET) $(WIN_CROSS_SERVER_TARGET) copy_assets_win_cross
	@echo "==> 壓縮 Windows 交叉編譯完整發布包至 $(PACKAGE_WIN_CROSS)..."
	@rm -f $(PACKAGE_WIN_CROSS)
	@cd $(WIN_CROSS_OUT_DIR) && zip -rq $(notdir $(PACKAGE_WIN_CROSS)) \
		$(notdir $(WIN_CROSS_TARGET)) $(notdir $(WIN_CROSS_SERVER_TARGET)) $(ASSETS_DIR)

# ==========================================
# [3] Windows 原生編譯流程 (build/win64_native/ - Windows CMD 執行)
# ==========================================
win64-native-all: win64-native-client win64-native-server copy_assets_win_nat package_win_nat
win64-native: win64-native-client copy_assets_win_nat package_win_nat

$(WIN_NAT_CLIENT_OBJ_DIR)/GirigaraRC.o: Girigara.rc
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(WIN_NAT_WINDRES) $< -o $@

win64-native-client: $(WIN_NAT_TARGET)
$(WIN_NAT_TARGET): $(WIN_NAT_CLIENT_OBJS) $(WIN_NAT_CLIENT_OBJ_DIR)/GirigaraRC.o
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [LD] 連結客戶端 $@ ...
	$(WIN_NAT_CXX) $(WIN_NAT_CLIENT_OBJS) $(WIN_NAT_CLIENT_OBJ_DIR)/GirigaraRC.o -o $@ $(WIN_NAT_LDFLAGS)

$(WIN_NAT_CLIENT_OBJ_DIR)/%.o: src/%.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(WIN_NAT_CXX) $(WIN_NAT_CXXFLAGS) -c $< -o $@

$(WIN_NAT_CLIENT_OBJ_DIR)/%.o: src/%.c
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(WIN_NAT_CC) $(WIN_NAT_CFLAGS) -c $< -o $@

win64-native-server: $(WIN_NAT_SERVER_TARGET)
$(WIN_NAT_SERVER_TARGET): $(WIN_NAT_SERVER_OBJS)
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [LD] 連結伺服器 $@ ...
	$(WIN_NAT_CXX) $(WIN_NAT_SERVER_OBJS) -o $@ $(WIN_NAT_SERVER_LDFLAGS)

$(WIN_NAT_SERVER_OBJ_DIR)/%.o: src/%.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(WIN_NAT_CXX) $(WIN_NAT_CXXFLAGS) -DGIRIGARA_SERVER -c $< -o $@


copy_assets_win_nat:
	@if not exist "$(subst /,\,$(WIN_NAT_OUT_DIR))" mkdir "$(subst /,\,$(WIN_NAT_OUT_DIR))"
	@if exist "$(subst /,\,$(WIN_NAT_OUT_DIR)\$(ASSETS_DIR))" rmdir /S /Q "$(subst /,\,$(WIN_NAT_OUT_DIR)\$(ASSETS_DIR))"
	@xcopy /E /I /Q /Y $(ASSETS_DIR) "$(subst /,\,$(WIN_NAT_OUT_DIR)\$(ASSETS_DIR))" >nul

package_win_nat: $(WIN_NAT_TARGET) $(WIN_NAT_SERVER_TARGET) copy_assets_win_nat
	@echo [PACKAGE] 打包 Windows 原生完整發布包至 $(PACKAGE_WIN_NAT) ...
	@if exist "$(subst /,\,$(PACKAGE_WIN_NAT))" del /Q "$(subst /,\,$(PACKAGE_WIN_NAT))"
	@cd $(subst /,\,$(WIN_NAT_OUT_DIR)) && tar -a -c -f "$(notdir $(PACKAGE_WIN_NAT))" \
		$(notdir $(WIN_NAT_TARGET)) $(notdir $(WIN_NAT_SERVER_TARGET)) $(ASSETS_DIR)
	@echo [OK] Windows 原生發布包打包完成！

# ==========================================
# RPM / Flatpak 打包目標 (Linux 專用)
# ==========================================
VERSION = 0.1.5

rpm:
	@mkdir -p ~/rpmbuild/SOURCES ~/rpmbuild/SPECS
	@git archive --format=tar.gz --prefix=ProjectGirigara-$(VERSION)/ -o ~/rpmbuild/SOURCES/ProjectGirigara-$(VERSION).tar.gz HEAD
	@cp ProjectGirigara.spec ~/rpmbuild/SPECS/
	@rpmbuild -ba ~/rpmbuild/SPECS/ProjectGirigara.spec

FLATPAK_ID        = com.hahenru.girigara
FLATPAK_MANIFEST  = $(FLATPAK_ID).yaml
FLATPAK_BUILD_DIR = build-flatpak
FLATPAK_REPO      = repo
FLATPAK_BUNDLE    = $(ROOT_BUILD_DIR)/$(FLATPAK_ID).flatpak

flatpak-install:
	@echo "==> 使用 flatpak-builder 編譯並安裝到本地..."
	flatpak-builder --user --install --force-clean $(FLATPAK_BUILD_DIR) $(FLATPAK_MANIFEST)

flatpak:
	@mkdir -p $(ROOT_BUILD_DIR)
	@echo "==> 編譯 Flatpak 儲存庫..."
	flatpak-builder --jobs=$(shell nproc) --repo=$(FLATPAK_REPO) --force-clean $(FLATPAK_BUILD_DIR) $(FLATPAK_MANIFEST)
	@echo "==> 匯出獨立安裝包至 $(FLATPAK_BUNDLE)..."
	flatpak build-bundle $(FLATPAK_REPO) $(FLATPAK_BUNDLE) $(FLATPAK_ID)
	@echo "==> Flatpak 打包完成！"

# ==========================================
# 清理與輔助工具 (跨平台相容)
# ==========================================
clean:
ifeq ($(HOST_OS),Windows)
	@echo 清理建置輸出目錄 (Windows)...
	@if exist $(ROOT_BUILD_DIR) rmdir /S /Q $(ROOT_BUILD_DIR)
else
	@echo "清理建置輸出目錄 (Linux)..."
	rm -rf $(ROOT_BUILD_DIR) $(FLATPAK_BUILD_DIR) $(FLATPAK_REPO)
endif

-include $(DEPS)

tree:
	$(TREE_CMD)

map:
	$(PYTHON) genProjectMap.py

stub:
	$(PYTHON) lua_binding_tool.py

# ==========================================
# 說明文件 (跨平台 Linux / Windows CMD 相容)
# ==========================================
help:
ifeq ($(HOST_OS),Windows)
	@echo ============================================================================
	@echo   Project Girigara - Makefile 編譯與打包指令總覽 (目前環境: $(HOST_OS))
	@echo ============================================================================
	@echo   [預設與三端全量編譯 (均包含 Client + Server + Tool + Assets + 打包)]
	@echo     make                  - 執行預設目標 ($(DEFAULT_GOAL))
	@echo     make all              - 建置 Linux 全端 + Windows 交叉編譯全端 + Flatpak
	@echo     make linux-all        - 建置 Linux 原生全端 (輸出至 $(LINUX_OUT_DIR)/)
	@echo     make win64-all        - 建置 Windows 交叉編譯全端 (輸出至 $(WIN_CROSS_OUT_DIR)/)
	@echo     make win64-native-all - 建置 Windows 原生全端 (輸出至 $(WIN_NAT_OUT_DIR)/)
	@echo.
	@echo   [單獨元件編譯 - Linux 原生]
	@echo     make linux-client     - 僅編譯 Linux Client
	@echo     make linux-server     - 僅編譯 Linux 無頭伺服器
	@echo.
	@echo   [單獨元件編譯 - Windows 交叉編譯 (Linux 端執行)]
	@echo     make win64-client     - 僅編譯 Windows Cross Client (.exe)
	@echo     make win64-server     - 僅編譯 Windows Cross 無頭伺服器 (.exe)
	@echo.
	@echo   [單獨元件編譯 - Windows 原生編譯 (Windows CMD 執行)]
	@echo     make win64-native-client - 僅編譯 Windows 原生 Client (.exe)
	@echo     make win64-native-server - 僅編譯 Windows 原生無頭伺服器 (.exe)
	@echo.
	@echo   [Linux 系統發行版打包 (v$(VERSION))]
	@echo     make rpm              - 建置 RPM 套件
	@echo     make flatpak          - 匯出獨立 .flatpak 安裝檔
	@echo     make flatpak-install  - 直接安裝 Flatpak 至本機使用者環境
	@echo.
	@echo   [開發輔助與清理]
	@echo     make map              - 執行 $(PYTHON) genProjectMap.py 更新簽名地圖
	@echo     make tree             - 匯出目前專案目錄樹至 tree.txt
	@echo     make clean            - 清除 build/ 與 Flatpak 建置快取
	@echo ============================================================================
else
	@printf '%s\n' \
	"============================================================================" \
	"  Project Girigara - Makefile 編譯與打包指令總覽 (目前環境: $(HOST_OS))" \
	"============================================================================" \
	"  [預設與三端全量編譯 (均包含 Client + Server + Tool + Assets + 打包)]" \
	"    make                  - 執行預設目標 ($(DEFAULT_GOAL))" \
	"    make all              - 建置 Linux 全端 + Windows 交叉編譯全端 + Flatpak" \
	"    make linux-all        - 建置 Linux 原生全端 (輸出至 $(LINUX_OUT_DIR)/)" \
	"    make win64-all        - 建置 Windows 交叉編譯全端 (輸出至 $(WIN_CROSS_OUT_DIR)/)" \
	"    make win64-native-all - 建置 Windows 原生全端 (輸出至 $(WIN_NAT_OUT_DIR)/)" \
	"" \
	"  [單獨元件編譯 - Linux 原生]" \
	"    make linux-client     - 僅編譯 Linux Client" \
	"    make linux-server     - 僅編譯 Linux 無頭伺服器" \
	"" \
	"  [單獨元件編譯 - Windows 交叉編譯 (Linux 端執行)]" \
	"    make win64-client     - 僅編譯 Windows Cross Client (.exe)" \
	"    make win64-server     - 僅編譯 Windows Cross 無頭伺服器 (.exe)" \
	"" \
	"  [單獨元件編譯 - Windows 原生編譯 (Windows CMD 執行)]" \
	"    make win64-native-client - 僅編譯 Windows 原生 Client (.exe)" \
	"    make win64-native-server - 僅編譯 Windows 原生無頭伺服器 (.exe)" \
	"" \
	"  [Linux 系統發行版打包 (v$(VERSION))]" \
	"    make rpm              - 建置 RPM 套件" \
	"    make flatpak          - 匯出獨立 .flatpak 安裝檔" \
	"    make flatpak-install  - 直接安裝 Flatpak 至本機使用者環境" \
	"" \
	"  [開發輔助與清理]" \
	"    make map              - 執行 $(PYTHON) genProjectMap.py 更新簽名地圖" \
	"    make tree             - 匯出目前專案目錄樹至 tree.txt" \
	"    make clean            - 清除 build/ 與 Flatpak 建置快取" \
	"============================================================================"
endif

.PHONY: all \
        linux-all linux linux-client linux-server copy_assets_linux package_linux \
        win64-all win64 win64-client win64-server copy_assets_win_cross package_win_cross \
        win64-native-all win64-native win64-native-client win64-native-server copy_assets_win_nat package_win_nat \
        rpm flatpak flatpak-install clean tree map help stub