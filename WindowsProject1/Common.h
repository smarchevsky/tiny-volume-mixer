#pragma once

#include <Windows.h>

#include <stdint.h>

inline void getBitmapData(HBITMAP hBmp, int& width, int& height, DWORD*& bits)
{
    bits = nullptr;
    if (hBmp) {
        DIBSECTION ds;
        GetObject(hBmp, sizeof(DIBSECTION), &ds);
        width = ds.dsBm.bmWidth;
        height = ds.dsBm.bmHeight;
        bits = (DWORD*)ds.dsBm.bmBits;
    }
}

inline HBITMAP getBitmapFromHDC(HDC hdc)
{
    return (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);
}

typedef DWORD PID;

enum {
    _____WM_APP = WM_APP,
    WM_REFRESH_VOL,
    WM_APP_REGISTERED,
    WM_APP_UNREGISTERED,
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