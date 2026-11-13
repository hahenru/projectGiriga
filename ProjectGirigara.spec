Name:           ProjectGirigara
Version:        0.1.5
Release:        0.1.alpha%{?dist}
Summary:        Girigara Game Engine Application
License:        Proprietary
URL:            https://github.com/Hahenru/ProjectGirigara
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  pkgconfig(freetype2)
BuildRequires:  glfw-devel
BuildRequires:  libGL-devel
BuildRequires:  lua-devel
BuildRequires:  alsa-lib-devel

# 執行期動態依賴 (可由 rpmbuild 自動分析，或明確標記)
Requires:       glfw
Requires:       lua
Requires:       freetype
Requires:       alsa-lib

%description
ProjectGirigara is a low-performance 2D game engine and GUI toolkit
written in C++23, leveraging OpenGL 4.3+ for modern graphics rendering.

Key Features:
- Lua script-driven UI system with live bindings
- Comprehensive event dispatching and pass-through mechanisms
- High-latency in-game console supporting UTF-8 and UTF-32 CJK fonts
- Complete RAII lifecycle management and modular system architecture

%prep
%autosetup

%build
# 呼叫既有的 Makefile 編譯 Linux 目標
%make_build linux

%install
rm -rf %{buildroot}

# 1. 建立安裝目錄結構
mkdir -p %{buildroot}/opt/%{name}
mkdir -p %{buildroot}/usr/bin

# 建立 512x512 圖示目錄並安裝
mkdir -p %{buildroot}/usr/share/icons/hicolor/512x512/apps
install -m 644 assets/icons/com.hahenru.girigara.png %{buildroot}/usr/share/icons/hicolor/512x512/apps/com.hahenru.girigara.png

# 2. 將執行檔與 assets 複製到 /opt/ProjectGirigara/
install -m 755 build/projectGIRIGARA %{buildroot}/opt/%{name}/projectGIRIGARA
cp -r assets %{buildroot}/opt/%{name}/assets

# 3. 建立封裝啟動腳本，確保執行時的工作目錄 (CWD) 切換到資源所在位置
cat << 'EOF' > %{buildroot}/usr/bin/projectGIRIGARA
#!/bin/bash
cd /opt/ProjectGirigara
exec ./projectGIRIGARA "$@"
EOF
chmod 755 %{buildroot}/usr/bin/projectGIRIGARA

# 同時建立小寫別名 symlink，方便終端機輸入
ln -sf projectGIRIGARA %{buildroot}/usr/bin/girigara

# 4. 安裝現有的標準 Desktop 捷徑
mkdir -p %{buildroot}/usr/share/applications
install -m 644 com.hahenru.girigara.desktop %{buildroot}/usr/share/applications/com.hahenru.girigara.desktop

%files
/opt/%{name}/projectGIRIGARA
/opt/%{name}/assets
/usr/bin/projectGIRIGARA
/usr/bin/girigara
/usr/share/applications/com.hahenru.girigara.desktop
/usr/share/icons/hicolor/512x512/apps/com.hahenru.girigara.png

%changelog
* Sun Oct 25 2026 Hahenru <hahenru@rucelin> - 1.0.0-1
- Initial RPM release for ProjectGirigara
