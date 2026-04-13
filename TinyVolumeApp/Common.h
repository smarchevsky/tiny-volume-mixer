#pragma once

#include <Windows.h>

#include <stdint.h>
#include <vector>

typedef DWORD PID;

enum {
    _____WM_APP = WM_APP,
    WM_REFRESH_VOL,
    WM_APP_REGISTERED,
    WM_APP_UNREGISTERED,
    WM_APP_ACTIVATION_CHANGED,
    WM_TIMER_UI,
};

enum class AlignUI : uint8_t {
    Left,
    Top,
    Right,
    Bottom,
    // don't change order above
    LeftTop,
    RightTop,
    LeftBottom,
    RightBottom
};

// run length encoded data
struct BufferRLE {
    std::vector<std::pair<BYTE, BYTE>> data;
};