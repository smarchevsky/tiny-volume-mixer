#pragma once

#include <Windows.h>

#include <stdint.h>

#define ARGB(a, r, g, b) ((DWORD(a) << 24) | (DWORD(r) << 16) | (DWORD(g) << 8) | DWORD(b))

#define ARGB_SPLIT(T, name, dval)       \
    T name##a = T((dval >> 24) & 0xFF), \
      name##r = T((dval >> 16) & 0xFF), \
      name##g = T((dval >> 8) & 0xFF),  \
      name##b = T((dval) & 0xFF);

#define LERP_BYTE_COLOR(result, col0, col1, x)                    \
    BYTE result##a = (col0##a + (col1##a - col0##a) * (x) / 255), \
         result##r = (col0##r + (col1##r - col0##r) * (x) / 255), \
         result##g = (col0##g + (col1##g - col0##g) * (x) / 255), \
         result##b = (col0##b + (col1##b - col0##b) * (x) / 255);

typedef DWORD PID;

enum {
    _____WM_APP = WM_APP,
    WM_REFRESH_VOL,
    WM_APP_REGISTERED,
    WM_APP_UNREGISTERED,
    WM_APP_ACTIVATION_CHANGED,
    WM_TIMER_UI,
};

enum class VolumeType : uint8_t {
    Invalid,
    Master,
    App
};

struct SelectInfo {
    SelectInfo(VolumeType type, PID pid) { _type = type, _pid = pid; }
    SelectInfo() = default;
    bool operator==(const SelectInfo& rhs) const { return _type == rhs._type && _pid == rhs._pid; }
    bool operator!=(const SelectInfo& rhs) const { return !operator==(rhs); }
    operator bool() const { return _type != VolumeType::Invalid; }
    PID _pid;
    VolumeType _type;
};

struct UIConfig {
    DWORD colorWindowFrame = 0xAAAAAAAA;
    DWORD colorWindowBck = 0xAA333333;
    uint8_t frameBorderWidth = 3;
    uint8_t sliderSpacing = 8;
    uint8_t sliderCornerRadius = 8;
    uint8_t sliderWidthApp = 80;
    uint8_t sliderWidthMaster = 100;
    uint8_t sliderBorderWidth = frameBorderWidth;
    uint8_t frameCornerRadius = sliderCornerRadius + sliderSpacing + frameBorderWidth;
    uint8_t iconSize = 48;
    uint8_t fontSize = 28;

    int getSliderOffsetL() const { return sliderSpacing / 2; }
    int getSliderOffsetR() const { return sliderSpacing - getSliderOffsetL(); }

    static UIConfig get()
    {
        UIConfig config;
        return config;
    }

private:
    UIConfig() = default;
};