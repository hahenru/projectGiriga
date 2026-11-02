# ==========================================
# Windows MinGW-w64 Makefile for Lua Static Lib
# ==========================================

# 編譯工具鏈設定
CC      = gcc
AR      = ar rcs
RANLIB  = ranlib

# 目標靜態庫檔名
LIB_NAME = liblua.a

# 安裝目標路徑 (指向你的 sysRoot)
SYSROOT     = C:/Users/Hartitska/sysRoot
INSTALL_INC = $(SYSROOT)/includes
INSTALL_LIB = $(SYSROOT)/libs

# 編譯旗標
CFLAGS  = -g

# 原始碼清單：包含所有 .c 但排除 lua.c (直譯器) 與 luac.c (位元組碼編譯器)
ALL_SRCS = $(wildcard src/*.c)
SRCS     = $(filter-out src/lua.c src/luac.c, $(ALL_SRCS))

# 對應的目標檔 (.o)
OBJS     = $(SRCS:.c=.o)

# 核心標頭檔清單
HEADERS  = src/lua.h src/luaconf.h src/lualib.h src/lauxlib.h src/lua.hpp

# 預設目標
all: $(LIB_NAME)

# 鏈結打包靜態庫
$(LIB_NAME): $(OBJS)
	@echo [AR] Packaging $@...
	$(AR) $@ $(OBJS)
	$(RANLIB) $@
	@echo [OK] Static library built successfully.

# 編譯各個 .c 檔為 .o
src/%.o: src/%.c
	@echo [CC] $<
	$(CC) $(CFLAGS) -c $< -o $@

# 一鍵安裝至 sysRoot
install: $(LIB_NAME)
	@echo [INSTALL] Installing headers to $(INSTALL_INC)...
	@if not exist "$(subst /,\,$(INSTALL_INC))" mkdir "$(subst /,\,$(INSTALL_INC))"
	@for %%f in ($(subst /,\,$(HEADERS))) do copy /Y "%%f" "$(subst /,\,$(INSTALL_INC))\" >nul
	@echo [INSTALL] Installing static library to $(INSTALL_LIB)...
	@if not exist "$(subst /,\,$(INSTALL_LIB))" mkdir "$(subst /,\,$(INSTALL_LIB))"
	@copy /Y "$(LIB_NAME)" "$(subst /,\,$(INSTALL_LIB))\" >nul
	@echo [OK] Lua installed to sysRoot successfully.

# 清除建構產物 (使用 Windows CMD 指令)
clean:
	@echo [CLEAN] Cleaning object files and library...
	@if exist src\*.o del /Q src\*.o
	@if exist $(LIB_NAME) del /Q $(LIB_NAME)
	@echo [OK] Clean done.

.PHONY: all install clean