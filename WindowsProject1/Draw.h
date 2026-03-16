#pragma once

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

#include <optional>

constexpr int borderR = 16;
#define RGBA(r, g, b, a) DWORD(((b) | (DWORD(g) << 8)) | ((DWORD(r)) << 16) | ((DWORD(a)) << 24))
#define ARGB(a, r, g, b) ((DWORD(a) << 24) | (DWORD(r) << 16) | (DWORD(g) << 8) | DWORD(b))
#define ARGBf(name, dval) float name##a = float((dval >> 24) & 0xFF), name##r = float((dval >> 16) & 0xFF), name##g = float((dval >> 8) & 0xFF), name##b = float((dval) & 0xFF);

struct Canvas {
    DWORD* pixels;
    LONG w, h;

    std::optional<RECT> customRegion;
    Canvas operator&(RECT rect)
    {
        Canvas result = *this;
        result.customRegion = rect;
        return result;
    }
};

void drawBorderedRect(const Canvas canvas, const RECT rc, int radius, int bw, DWORD bg_col, DWORD bo_col);