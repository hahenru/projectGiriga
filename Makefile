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
# 1. 雙端共用核心 (Common)
COMMON_SRC = \
    src/LoggerSystem.cpp \
    src/Core/CoreContext.cpp \
    src/Network/ENetNode.cpp \
    src/Scene.cpp \
    src/SceneManager.cpp \
    src/LuaScene.cpp \
    src/ScriptEngine.cpp \
    src/ScriptResource.cpp \
    src/SettingsRegistry.cpp \
    src/ConsoleSystem.cpp \
    src/Girigara.cpp \
    src/main.cpp

# 2. 客戶端專屬 (Client)
CLIENT_SRC = \
    src/Client/ClientContext.cpp \
    src/Client/ClientResourceManager.cpp \
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
    src/FontResource.cpp \
    src/AudioResource.cpp \
    src/miniaudio_impl.cpp \
    src/AudioSystem.cpp \
    src/UIPanel.cpp \
    src/UIHorizontalLayout.cpp \
    src/UISettingsMenu.cpp \
    src/Client/ClientRuntime.cpp

SERVER_SRC = \
    src/Server/ServerContext.cpp \
    src/Server/ServerResourceManager.cpp \
    src/Server/ServerRuntime.cpp
    # src/Server/ServerApp.cpp \
    # src/Server/server_main.cpp

ALL_CLIENT_SRC = $(COMMON_SRC) $(CLIENT_SRC) $(SERVER_SRC)
ALL_SERVER_SRC = $(COMMON_SRC) $(SERVER_SRC)

# ==========================================
# 原生 Linux 工具鏈與旗標
# ==========================================
CXX = g++
CC = gcc
CXXFLAGS = -g -std=c++23 -MMD -MP -fPIE -I./include $(shell pkg-config --cflags freetype2 2>/dev/null)
CFLAGS = -g -MMD -MP -fPIE -I./include

# Linux 連結參數 (加入 -lenet)
LDFLAGS = -lglfw -lGL -ldl -llua -lpthread -lasound -lm -lenet $(shell pkg-config --libs freetype2 2>/dev/null)
SERVER_LDFLAGS = -ldl -llua -lpthread -lm -lenet

BUILD_DIR = build
SERVER_BUILD_DIR = $(BUILD_DIR)/server
ASSETS_DIR = assets
TARGET = $(BUILD_DIR)/projectGIRIGARA
SERVER_TARGET = $(BUILD_DIR)/projectGIRIGARA_server
PACKAGE_LINUX = $(BUILD_DIR)/ProjectGirigara_linux.tar.gz

# ==========================================
# MinGW-w64 Windows 交叉編譯設定 (由 Linux 端執行)
# ==========================================
# 定義資源編譯器
WIN_WINDRES = x86_64-w64-mingw32-windres

WIN_CXX = x86_64-w64-mingw32-g++
WIN_CC  = x86_64-w64-mingw32-gcc
WIN_PREFIX = /usr/x86_64-w64-mingw32/sys-root/mingw

WIN_CXXFLAGS = -O2 -std=c++23 -MMD -MP -I./include -I$(WIN_PREFIX)/include/freetype2
WIN_CFLAGS   = -O2 -MMD -MP -I./include

# Windows 交叉編譯連結參數 (加入 -lenet -lws2_32)
WIN_LDFLAGS  = -mwindows -Wl,--subsystem,windows \
               -L$(WIN_PREFIX)/lib \
               -lglfw3 -llua -lopengl32 -lole32 \
               -lfreetype -lpng -lz -lbz2 \
               -lgdi32 -limm32 -lpthread \
               -lenet -lws2_32 -lwinmm \
               -static-libgcc -static-libstdc++ -static

WIN_BUILD_DIR = $(BUILD_DIR)/win64
WIN_TARGET = $(WIN_BUILD_DIR)/projectGIRIGARA.exe
PACKAGE_WIN = $(BUILD_DIR)/ProjectGirigara_win64.zip

# ==========================================
# 原生 Windows (MinGW-w64) 編譯設定 (由 Windows 端執行)
# ==========================================
WIN_NAT_CXX = g++
WIN_NAT_CC  = gcc
WIN_NAT_CXXFLAGS = -g -std=c++23 -I./include
WIN_NAT_CFLAGS   = -g -I./include

# Windows 原生連結參數 (加入 -lenet -lws2_32，注意順序 -lenet 在前)
WIN_NAT_LDFLAGS  = -mwindows -Wl,--subsystem,windows \
    -lglfw3 -lopengl32 -llua -lfreetype -lgdi32 -lpthread -lole32 \
    -lenet -lws2_32 -lwinmm

WIN_NAT_SERVER_LDFLAGS = -llua -lpthread -lenet -lws2_32 -lwinmm

WIN_NAT_BUILD_DIR = $(BUILD_DIR)/win64_native
WIN_NAT_SERVER_BUILD_DIR = $(BUILD_DIR)/win64_native_server
WIN_NAT_TARGET    = $(WIN_NAT_BUILD_DIR)/projectGIRIGARA.exe
WIN_NAT_SERVER_TARGET = $(WIN_NAT_BUILD_DIR)/projectGIRIGARA_server.exe
PACKAGE_WIN_NAT   = $(BUILD_DIR)/ProjectGirigara_win64_native.zip

# ==========================================
# 目標檔路徑映射
# ==========================================
OBJS = $(patsubst src/%.cpp, $(BUILD_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
       $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(filter %.c, $(ALL_CLIENT_SRC)))

SERVER_OBJS = $(patsubst src/%.cpp, $(SERVER_BUILD_DIR)/%.o, $(filter %.cpp, $(ALL_SERVER_SRC)))

WIN_OBJS = $(patsubst src/%.cpp, $(WIN_BUILD_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
           $(patsubst src/%.c, $(WIN_BUILD_DIR)/%.o, $(filter %.c, $(ALL_CLIENT_SRC)))

WIN_NAT_OBJS = $(patsubst src/%.cpp, $(WIN_NAT_BUILD_DIR)/%.o, $(filter %.cpp, $(ALL_CLIENT_SRC))) \
               $(patsubst src/%.c, $(WIN_NAT_BUILD_DIR)/%.o, $(filter %.c, $(ALL_CLIENT_SRC)))

WIN_NAT_SERVER_OBJS = $(patsubst src/%.cpp, $(WIN_NAT_SERVER_BUILD_DIR)/%.o, $(filter %.cpp, $(ALL_SERVER_SRC)))

DEPS = $(OBJS:.o=.d) $(SERVER_OBJS:.o=.d) $(WIN_OBJS:.o=.d)

# ==========================================
# Linux 原生建置流程
# ==========================================
all: linux win64 flatpak

linux: $(TARGET) copy_assets_linux package_linux

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

server: $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(SERVER_OBJS) -o $@ $(SERVER_LDFLAGS)

$(SERVER_BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DGIRIGARA_SERVER -c $< -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

copy_assets_linux:
	@mkdir -p $(BUILD_DIR)
	@rm -rf $(BUILD_DIR)/$(ASSETS_DIR)
	@cp -r $(ASSETS_DIR) $(BUILD_DIR)/

package_linux: $(TARGET) copy_assets_linux
	@echo "==> 打包 Linux 發布包至 $(PACKAGE_LINUX) ..."
	@tar -czf $(PACKAGE_LINUX) -C $(BUILD_DIR) $(notdir $(TARGET)) $(ASSETS_DIR)

# ==========================================
# Windows 交叉編譯流程 (在 Linux 主機上執行)
# ==========================================
win64: $(WIN_TARGET) copy_assets_win package_win

# 將 .rc 編譯成 win32.o 資源物件
build/win64/GirigaraRC.o: Girigara.rc
	@mkdir -p $(dir $@)
	$(WIN_WINDRES) $< -o $@


$(WIN_TARGET): $(WIN_OBJS) build/win64/GirigaraRC.o
	@mkdir -p $(dir $@)
	@echo "==> 靜態鏈結 $(WIN_TARGET) ..."
	$(WIN_CXX) $(WIN_OBJS) build/win64/GirigaraRC.o -o $@ $(WIN_LDFLAGS)

$(WIN_BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(WIN_CXX) $(WIN_CXXFLAGS) -c $< -o $@

$(WIN_BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(WIN_CC) $(WIN_CFLAGS) -c $< -o $@

copy_assets_win:
	@mkdir -p $(WIN_BUILD_DIR)
	@echo "==> 複製 assets 至 Windows 目錄..."
	@rm -rf $(WIN_BUILD_DIR)/$(ASSETS_DIR)
	@cp -r $(ASSETS_DIR) $(WIN_BUILD_DIR)/

package_win: $(WIN_TARGET) copy_assets_win
	@echo "==> 壓縮 Windows 發布包至 $(PACKAGE_WIN)..."
	@rm -f $(PACKAGE_WIN)
	@cd $(WIN_BUILD_DIR) && zip -rq ../../$(PACKAGE_WIN) .
	@echo "==> Windows x86_64 發布包打包完成！"

# ==========================================
# Windows 原生編譯流程 (在 Windows 主機 CMD 下執行)
# ==========================================
win64-native: package_win_nat

$(WIN_NAT_TARGET): $(WIN_NAT_OBJS)
	@echo [LD] 連結 $@ ...
	$(WIN_NAT_CXX) $(WIN_NAT_OBJS) -o $@ $(WIN_NAT_LDFLAGS)

win64-native-server: $(WIN_NAT_SERVER_OBJS)
	@if not exist "$(WIN_NAT_BUILD_DIR)" mkdir "$(WIN_NAT_BUILD_DIR)"
	@echo [LD] 連結無頭伺服器 $(WIN_NAT_SERVER_TARGET) ...
	$(WIN_NAT_CXX) $(WIN_NAT_SERVER_OBJS) -o $(WIN_NAT_SERVER_TARGET) $(WIN_NAT_SERVER_LDFLAGS)

$(WIN_NAT_SERVER_BUILD_DIR)/%.o: src/%.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [CXX-SERVER] $<
	$(WIN_NAT_CXX) $(WIN_NAT_CXXFLAGS) -DGIRIGARA_SERVER -c $< -o $@
	
$(WIN_NAT_BUILD_DIR)/%.o: src/%.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [CXX] $<
	$(WIN_NAT_CXX) $(WIN_NAT_CXXFLAGS) -c $< -o $@

$(WIN_NAT_BUILD_DIR)/%.o: src/%.c
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	@echo [CC] $<
	$(WIN_NAT_CC) $(WIN_NAT_CFLAGS) -c $< -o $@

copy_assets_win_nat:
	@if not exist "$(WIN_NAT_BUILD_DIR)" mkdir "$(WIN_NAT_BUILD_DIR)"
	@echo [ASSETS] 複製 assets 資料夾...
	@if exist "$(WIN_NAT_BUILD_DIR)\$(ASSETS_DIR)" rmdir /S /Q "$(WIN_NAT_BUILD_DIR)\$(ASSETS_DIR)"
	@xcopy /E /I /Q /Y $(ASSETS_DIR) "$(WIN_NAT_BUILD_DIR)\$(ASSETS_DIR)" >nul

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

FLATPAK_ID = com.hahenru.girigara
FLATPAK_MANIFEST = $(FLATPAK_ID).yaml
FLATPAK_BUILD_DIR = build-flatpak
FLATPAK_REPO = repo
FLATPAK_BUNDLE = $(BUILD_DIR)/$(FLATPAK_ID).flatpak

flatpak-install:
	@echo "==> 使用 flatpak-builder 編譯並安裝到本地..."
	flatpak-builder --user --install --force-clean $(FLATPAK_BUILD_DIR) $(FLATPAK_MANIFEST)

flatpak:
	@mkdir -p $(BUILD_DIR)
	@echo "==> 編譯 Flatpak 儲存庫..."
	flatpak-builder --repo=$(FLATPAK_REPO) --force-clean $(FLATPAK_BUILD_DIR) $(FLATPAK_MANIFEST)
	@echo "==> 匯出獨立安裝包至 $(FLATPAK_BUNDLE)..."
	flatpak build-bundle $(FLATPAK_REPO) $(FLATPAK_BUNDLE) $(FLATPAK_ID)
	@echo "==> Flatpak 打包完成！"

# ==========================================
# 清理規則 (跨平台相容)
# ==========================================
clean:
ifeq ($(HOST_OS),Windows)
	@echo 清理建置輸出目錄 (Windows)...
	@if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
else
	@echo "清理建置輸出目錄 (Linux)..."
	rm -rf $(BUILD_DIR) $(FLATPAK_BUILD_DIR) $(FLATPAK_REPO)
endif

-include $(DEPS)

tree:
	tree . /f

.PHONY: all linux server win64 win64-native win64-native-server \
        copy_assets_linux package_linux copy_assets_win package_win \
        copy_assets_win_nat package_win_nat clean rpm flatpak flatpak-install tree
