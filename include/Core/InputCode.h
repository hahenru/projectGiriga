#ifndef GIRIGARA_CORE_INPUTCODE_H
#define GIRIGARA_CORE_INPUTCODE_H

namespace InputCode {
    constexpr int ActionRelease = 0;
    constexpr int ActionPress   = 1;
    constexpr int ActionRepeat  = 2;

    constexpr int ModShift      = 0x0001;
    constexpr int ModControl    = 0x0002;
    constexpr int ModAlt        = 0x0004;
    constexpr int ModSuper      = 0x0008;

    constexpr int KeyEscape     = 256;
    constexpr int KeyGraveAccent = 96;
}

#endif // GIRIGARA_CORE_INPUTCODE_H