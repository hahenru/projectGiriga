#ifndef GIRIGARA_INLINE_PATHDECODE_H
#define GIRIGARA_INLINE_PATHDECODE_H
#include <filesystem>
#include <string>
#include <string_view>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

inline std::filesystem::path pathFromUtf8(std::string_view utf8Str) {
#if defined(_WIN32)
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()), nullptr, 0);
    if (len <= 0) return {};

    std::wstring wide;
    wide.resize_and_overwrite(len, [&](wchar_t* buf, size_t n) {
        MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()), buf, static_cast<int>(n));
        return n;
    });
    return std::filesystem::path(std::move(wide));
#else
    return std::filesystem::path(utf8Str);
#endif
}

inline std::string pathToUtf8(const std::filesystem::path& p) {
#if defined(_WIN32)
    const auto& wide = p.native();
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};

    std::string utf8;
    utf8.resize_and_overwrite(len, [&](char* buf, size_t n) {
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), buf, static_cast<int>(n), nullptr, nullptr);
        return n;
    });
    return utf8;
#else
    return p.string();
#endif
}

#endif//GIRIGARA_INLINE_PATHDECODE_H