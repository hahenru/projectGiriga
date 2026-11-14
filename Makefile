# ==========================================
# 跨平台作業系統環境偵測
# ==========================================
ifeq ($(OS),Windows_NT)
    HOST_OS = Windows
    DEFAULT_GOAL = win64-native
else
    HOST_OS = Linux
    DEFAULT_GOAL = all
endif

.DEFAULT_GOAL := $(DEFAULT_GOAL)

# ==========================================
# 原始碼分類 (嚴格依據目錄結構分流)
# ==========================================
# 1. 三端共用核心 (Common - 零網路、零音訊、零視窗依賴)
COMMON_SRC = \
    src/LoggerSystem.cpp \
    src/Core/CoreContext.cpp \
    src/Scene.cpp \
    src/SceneManager.cpp \
    src/LuaScene.cpp \
    src/ScriptEngine.cpp \
    src/ScriptResource.cpp \
    src/ConsoleSystem.cpp \
    src/GpkArchive.cpp \
    src/Girigara.cpp \
    src/main.cpp

# 2. 網路模組 (Net - 供 Client 與 Server 共用，Tool 不編譯)
NET_SRC = \
    src/Network/ENetNode.cpp

# 2. 客戶端專屬 (Client)
CLIENT_SRC = \
    src/Client/ClientContext.cpp \
    src/Client/ClientResourceManager.cpp \
    src/Client/ClientRuntime.cpp \
    src/ImageResource.cpp \
    src/glad.c \
    src/Shader.cpp \
    src/WindowManager.cpp \
    src/UIContainer.cpp \
    src/UIElement.cpp \
    src/UIButton.cpp \
    src/UIText.cpp \
    src/UIConsole.cpp \
    src/QuadMesh.cpp \
    src/UIWindow.cpp \
    src/KeyboardSystem.cpp \
    src/MainMenuScene.cpp \
    src/GameScene.cpp \
    src/FontLoader.cpp \
    src/UIVerticalLayout.cpp \
    src/UISlantedLayout.cpp \
    src/UIRenderer.cpp \
    src/UIManager.cpp \
    src/UIFactory.cpp \
    src/FontResource.cpp \
    src/AudioResource.cpp \
    src/miniaudio_impl.cpp \
    src/stb_image_impl.cpp \
    src/AudioSystem.cpp \
    src/UIPanel.cpp \
    src/UIHorizontalLayout.cpp \
    src/SettingsRegistry.cpp \
    src/UISettingsMenu.cpp

# 3. 伺服器專屬 (Server)
SERVER_SRC = \
    src/Server/ServerContext.cpp \
    src/Server/ServerResourceManager.cpp \
    src/Server/ServerRuntime.cpp \
    src/Server/ServerConfig.cpp

# 4. 獨立工具專屬 (Tool - 零網路、零音訊)
TOOL_SRC = \
    src/Tool/ToolContext.cpp \
    src/Tool/ToolResourceManager.cpp \
    src/Tool/ToolRuntime.cpp \
    src/GpkScriptBinder.cpp \
    src/ImageResource.cpp \
    src/glad.c \
    src/Shader.cpp \
    src/WindowManager.cpp \
    src/UIContainer.cpp \
    src/UIElement.cpp \
    src/UIButton.cpp \
    src/UIText.cpp \
    src/UIConsole.cpp \
    src/QuadMesh.cpp \
    src/UIWindow.cpp \
    src/KeyboardSystem.cpp \
    src/FontLoader.cpp \
    src/UIVerticalLayout.cpp \
    src/UISlantedLayout.cpp \
    src/UIRenderer.cpp \
    src/UIManager.cpp \
    src/UIFactory.cpp \
    src/FontResource.cpp \
    src/stb_image_impl.cpp \
    src/UIPanel.cpp \
    src/UIHorizontalLayout.cpp \
    src/UISettingsMenu.cpp

# 預設整合版 Client（含本機伺服器能力）
ALL_CLIENT_SRC      = $(COMMON_SRC) $(NET_SRC) $(CLIENT_SRC) $(SERVER_SRC)
# 純客戶端（只能連線遠端伺服器，無法開本地服，體積更小）
ALL_PURE_CLIENT_SRC = $(COMMON_SRC) $(NET_SRC) $(CLIENT_SRC)
# 獨立伺服器
ALL_SERVER_SRC      = $(COMMON_SRC) $(NET_SRC) $(SERVER_SRC)
# 獨立打包工具
ALL_TOOL_SRC        = $(COMMON_SRC) $(TOOL_SRC)

# ==========================================
# 目錄結構與產出路徑集中定義
# ==========================================
ROOT_BUILD_DIR    = build
ASSETS_DIR        = assets

# Linux 統一輸出根目錄
LINUX_OUT_DIR     = $(ROOT_BUILD_DIR)/linux
CLIENT_OBJ_DIR    = $(LINUX_OUT_DIR)/Girigara
SERVER_OBJ_DIR    = $(LINUX_OUT_DIR)/Server
TOOL_OBJ_DIR      = $(LINUX_OUT_DIR)/GpkTool

# 各自的中繼目標檔 (.o) 先編在獨立資料夾，連結時直接輸出至 $(LINUX_OUT_DIR)
TARGET            = $(LINUX_OUT_DIR)/projectGIRIGARA
SERVER_TARGET     = $(LINUX_OUT_DIR)/projectGIRIGARA_server
TOOL_TARGET       = $(LINUX_OUT_DIR)/projectGIRIGARA_tool
PACKAGE_LINUX     = $(LINUX_OUT_DIR)/ProjectGirigara_linux.tar.gz

# Windows 交叉編譯輸出目錄 (位於 build/linux/win64Cross)
WIN_OUT_DIR       = $(LINUX_OUT_DIR)/win64Cross
WIN_OBJ_DIR       = $(WIN_OUT_DIR)/obj
WIN_TARGET        = $(WIN_OUT_DIR)/projectGIRIGARA.exe
PACKAGE_WIN       = $(WIN_OUT_DIR)/ProjectGirigara_win64.zip

# Windows 原生編譯輸出目錄 (Windows 端 CMD 執行)
WIN_NAT_OUT_DIR        = $(ROOT_BUILD_DIR)/win64_native
WIN_NAT_SERVER_OBJ_DIR = $(WIN_NAT_OUT_DIR)/Server
WIN_NAT_TARGET         = $(WIN_NAT_OUT_DIR)/projectGIRIGARA.exe
WIN_NAT_SERVER_TARGET  = $(WIN_NAT_OUT_DIR)/projectGIRIGARA_server.exe
PACKAGE_WIN_NAT        = $(ROOT_BUILD_DIR)/ProjectGirigara_win64_native.zip

# ==========================================
# 原生 Linux 工具鏈與旗標
# ==========================================
CXX = g++
CC  = gcc
CXXFLAGS = -g -std=c++23 -MMD -MP -fPIE -I./include $(shell pkg-config --cflags freetype2 miniz 2>/dev/null)
CFLAGS   = -g -MMD -MP -fPIE -I./include

LDFLAGS        = -lglfw -lGL -ldl -llua -lpthread -lasound -lm -lenet -lminiz $(shell pkg-config --libs freetype2 miniz 2>/dev/null)
SERVER_LDFLAGS = -ldl -llua -lpthread -lm -lenet -lminiz $(shell pkg-config --libs miniz 2>/dev/null)
TOOL_LDFLAGS   = -lglfw -lGL -ldl -llua -lpthread -lm -lminiz $(shell pkg-config --libs freetype2 miniz 2>/dev/null)

# ==========================================
# MinGW-w64 Windows 交叉編譯設定 (Linux 端執行)
# ==========================================
WIN_WINDRES = x86_64-w64-mingw32-windres
WIN_CXX     = x86_64-w64-mingw32-g++
WIN_CC      = x86_64-w64-mingw32-gcc
WIN_PREFIX  = /usr/x86_64-w64-mingw32/sys-root/mingw

WIN_CXXFLAGS = -O2 -std=c++23 -MMD -MP -I./include -I$(WIN_PREFIX)/include/freetype2
WIN_CFLAGS   = -O2 -MMD -MP -I./include

WIN_LDFLAGS  = -mwindows -Wl,--subsystem,windows \
               -L$(WIN_PREFIX)/lib \
               -lglfw3 -llua -lopengl32 -lole32 \
               -lfreetype -lpng -lminiz -lz -lbz2 \
               -lgdi32 -limm32 -lpthread \
               -lenet -lws2_32 -lwinmm \
               -static-libgcc -static-libstdc++ -static

# ==========================================
# 原生 Windows (MinGW-w64) 編譯設定
# ==========================================
WIN_NAT_CXX = g++
WIN_NAT_CC  = gcc
WIN_NAT_CXXFLAGS = -g -std=c++23 -I./include
WIN_NAT_CFLAGS   = -g -I./include

WIN_NAT_LDFLAGS  = -mwindows -Wl,--subsystem,windows \
    -lglfw3 -lopengl32 -llua -lfreetype -lgdi32 -lpthread -lole32 \
    -lenet -lws2_32 -lwinmm -lminiz

WIN_NAT_SERVER_LDFLAGS = -llua -lpthread -lenet -lws2_32 -lwinmm -lminiz

# ==========================================
# 目標檔路徑映射
# ==========================================
OBJS = $(patsubst src/%.cpp, $(CLIENT_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
       $(patsubst src/%.c, $(CLIENT_OBJ_DIR)/%.o, $(filter %.c, $(ALL_CLIENT_SRC)))

SERVER_OBJS = $(patsubst src/%.cpp, $(SERVER_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_SERVER_SRC)))

TOOL_OBJS = $(patsubst src/%.cpp, $(TOOL_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_TOOL_SRC))) \
            $(patsubst src/%.c, $(TOOL_OBJ_DIR)/%.o, $(filter %.c, $(ALL_TOOL_SRC)))

WIN_OBJS = $(patsubst src/%.cpp, $(WIN_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
           $(patsubst src/%.c, $(WIN_OBJ_DIR)/%.o, $(filter %.c, $(ALL_CLIENT_SRC)))

WIN_NAT_OBJS = $(patsubst src/%.cpp, $(WIN_NAT_OUT_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
               $(patsubst src/%.c, $(WIN_NAT_OUT_DIR)/%.o, $(filter %.c, $(ALL_CLIENT_SRC)))

WIN_NAT_SERVER_OBJS = $(patsubst src/%.cpp, $(WIN_NAT_SERVER_OBJ_DIR)/%.o, $(filter %.cpp, $(ALL_SERVER_SRC)))

DEPS = $(OBJS:.o=.d) $(SERVER_OBJS:.o=.d) $(TOOL_OBJS:.o=.d) $(WIN_OBJS:.o=.d)

# ==========================================
# Linux 原生建置流程 (全輸出匯集於 build/linux/)
# ==========================================
all: linux-all win64 flatpak

# 一次建置 Linux 端的 Client + Server + Tool + 共用 assets
linux-all: linux server tool

linux: $(TARGET) copy_assets_linux package_linux

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(CLIENT_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CLIENT_OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

server: $(SERVER_TARGET) copy_assets_linux

$(SERVER_TARGET): $(SERVER_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(SERVER_OBJS) -o $@ $(SERVER_LDFLAGS)

$(SERVER_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DGIRIGARA_SERVER -c $< -o $@

tool: $(TOOL_TARGET) copy_assets_linux

$(TOOL_TARGET): $(TOOL_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(TOOL_OBJS) -o $@ $(TOOL_LDFLAGS)

$(TOOL_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DGIRIGARA_TOOL -c $< -o $@

$(TOOL_OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DGIRIGARA_TOOL -c $< -o $@

copy_assets_linux:
	@mkdir -p $(LINUX_OUT_DIR)
	@rm -rf $(LINUX_OUT_DIR)/$(ASSETS_DIR)
	@cp -r $(ASSETS_DIR) $(LINUX_OUT_DIR)/

package_linux: $(TARGET) copy_assets_linux
	@echo "==> 打包 Linux 發布包至 $(PACKAGE_LINUX) ..."
	@tar -czf $(PACKAGE_LINUX) -C $(LINUX_OUT_DIR) $(notdir $(TARGET)) $(ASSETS_DIR)

# ==========================================
# Windows 交叉編譯流程 (輸出至 build/linux/win64Cross/)
# ==========================================
win64: $(WIN_TARGET) copy_assets_win package_win

$(WIN_OBJ_DIR)/GirigaraRC.o: Girigara.rc
	@mkdir -p $(dir $@)
	$(WIN_WINDRES) $< -o $@

$(WIN_TARGET): $(WIN_OBJS) $(WIN_OBJ_DIR)/GirigaraRC.o
	@mkdir -p $(dir $@)
	@echo "==> 靜態鏈結 $(WIN_TARGET) ..."
	$(WIN_CXX) $(WIN_OBJS) $(WIN_OBJ_DIR)/GirigaraRC.o -o $@ $(WIN_LDFLAGS)

$(WIN_OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@

$(WIN_OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(WIN_CC) $(WIN_CFLAGS) -c $< -o $@

copy_assets_win:
	@mkdir -p $(WIN_OUT_DIR)
	@echo "==> 複製 assets 至 $(WIN_OUT_DIR)..."
	@rm -rf $(WIN_OUT_DIR)/$(ASSETS_DIR)
	@cp -r $(ASSETS_DIR) $(WIN_OUT_DIR)/

package_win: $(WIN_TARGET) copy_assets_win
	@echo "==> 壓縮 Windows 發布包至 $(PACKAGE_WIN)..."
	@rm -f $(PACKAGE_WIN)
	@cd $(WIN_OUT_DIR) && zip -rq $(notdir $(PACKAGE_WIN)) $(notdir $(WIN_TARGET)) $(ASSETS_DIR)
	@echo "==> Windows x86_64 發布包打包完成！"

# ==========================================
# Windows 原生編譯流程 (在 Windows 主機 CMD 下執行)
# ==========================================
win64-native: package_win_nat

$(WIN_NAT_TARGET): $(WIN_NAT_OBJS)
	@echo [LD] 連結 $@ ...
	$(WIN_NAT_CXX) $(WIN_NAT_OBJS) -o $@ $(WIN_NAT_LDFLAGS)

win64-native-server: $(WIN_NAT_SERVER_OBJS)
	@if not exist "$(subst /,\,$(WIN_NAT_OUT_DIR))" mkdir "$(subst /,\,$(WIN_NAT_OUT_DIR))"
	@echo [LD] 連結無頭伺服器 $(WIN_NAT_SERVER_TARGET) ...
	$(WIN_NAT_CXX) $(WIN_NAT_SERVER_OBJS) -o $(WIN_NAT_SERVER_TARGET) $(WIN_NAT_SERVER_LDFLAGS)

$(WIN_NAT_SERVER_OBJ_DIR)/%.o: src/%.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [CXX-SERVER] $<
	$(WIN_NAT_CXX) $(WIN_NAT_CXXFLAGS) -DGIRIGARA_SERVER -c $< -o $@

$(WIN_NAT_OUT_DIR)/%.o: src/%.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [CXX] $<
	$(WIN_NAT_CXX) $(WIN_NAT_CXXFLAGS) -c $< -o $@

$(WIN_NAT_OUT_DIR)/%.o: src/%.c
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [CC] $<
	$(WIN_NAT_CC) $(WIN_NAT_CFLAGS) -c $< -o $@

copy_assets_win_nat:
	@if not exist "$(subst /,\,$(WIN_NAT_OUT_DIR))" mkdir "$(subst /,\,$(WIN_NAT_OUT_DIR))"
	@echo [ASSETS] 複製 assets 資料夾...
	@if exist "$(subst /,\,$(WIN_NAT_OUT_DIR)\$(ASSETS_DIR))" rmdir /S /Q "$(subst /,\,$(WIN_NAT_OUT_DIR)\$(ASSETS_DIR))"
	@xcopy /E /I /Q /Y $(ASSETS_DIR) "$(subst /,\,$(WIN_NAT_OUT_DIR)\$(ASSETS_DIR))" >nul

package_win_nat: $(WIN_NAT_TARGET) copy_assets_win_nat
	@echo [PACKAGE] 打包 Windows 發布包至 $(PACKAGE_WIN_NAT) ...
	@if exist "build\ProjectGirigara_win64_native.zip" del /Q "build\ProjectGirigara_win64_native.zip"
	@cd build\win64_native && tar -a -c -f "..\ProjectGirigara_win64_native.zip" projectGIRIGARA.exe assets
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

FLATPAK_ID       = com.hahenru.girigara
FLATPAK_MANIFEST = $(FLATPAK_ID).yaml
FLATPAK_BUILD_DIR = build-flatpak
FLATPAK_REPO     = repo
FLATPAK_BUNDLE   = $(ROOT_BUILD_DIR)/$(FLATPAK_ID).flatpak

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
# 清理規則 (跨平台相容)
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
	tree . /f

.PHONY: all linux-all linux server tool win64 win64-native win64-native-server \
        copy_assets_linux package_linux copy_assets_win package_win \
        copy_assets_win_nat package_win_nat clean rpm flatpak flatpak-install tree
