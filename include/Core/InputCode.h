#ifndef GIRIGARA_CORE_INPUTCODE_H
#define GIRIGARA_CORE_INPUTCODE_H

namespace InputCode {
    // 動作狀態 (對齊 GLFW / Android KeyEvent 語意)
    constexpr int ActionRelease = 0;
    constexpr int ActionPress   = 1;
    constexpr int ActionRepeat  = 2;

    // 修飾鍵 Bitmask
    constexpr int ModShift      = 0x0001;
    constexpr int ModControl    = 0x0002;
    constexpr int ModAlt        = 0x0004;
    constexpr int ModSuper      = 0x0008;

    // 滑鼠按鍵 (未來 Android 觸控主手指直接映射 MouseLeft)
    constexpr int MouseLeft     = 0;
    constexpr int MouseRight    = 1;
    constexpr int MouseMiddle   = 2;

    // 常用編輯與導航鍵 (給 UIInputField / UIConsole / UISettingsMenu / UIWindow 用)
    constexpr int KeySpace      = 32;
    constexpr int KeyGraveAccent = 96; // ` / ~ 喚出 Console

    constexpr int KeyEscape     = 256; // Android 返回鍵 (AKEYCODE_BACK) 可直接映射至此
    constexpr int KeyEnter      = 257;
    constexpr int KeyTab        = 258;
    constexpr int KeyBackspace  = 259;
    constexpr int KeyInsert     = 260;
    constexpr int KeyDelete     = 261;
    constexpr int KeyRight      = 262;
    constexpr int KeyLeft       = 263;
    constexpr int KeyDown       = 264;
    constexpr int KeyUp         = 265;
    constexpr int KeyPageUp     = 266;
    constexpr int KeyPageDown   = 267;
    constexpr int KeyHome       = 268;
    constexpr int KeyEnd        = 269;

    // 常用快捷鍵字母 (Ctrl+C / V / X / A / Z)
    constexpr int KeyA          = 65;
    constexpr int KeyC          = 67;
    constexpr int KeyV          = 86;
    constexpr int KeyX          = 88;
    constexpr int KeyZ          = 90;
}

#endif // GIRIGARA_CORE_INPUTCODE_H
