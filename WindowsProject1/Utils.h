#pragma once

#include "Common.h"

#include "Draw.h"

#include <string>

inline HBITMAP getBitmapFromHDC(HDC hdc) { return (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP); }
inline void getBitmapData(HBITMAP hBmp, SIZE& size, DWORD*& bits)
{
    bits = nullptr;
    if (hBmp) {
        DIBSECTION ds;
        GetObject(hBmp, sizeof(DIBSECTION), &ds);
        size.cx = ds.dsBm.bmWidth;
        size.cy = ds.dsBm.bmHeight;
        bits = (DWORD*)ds.dsBm.bmBits;
    }
}

inline BITMAPINFO getBMI_ARGB(SIZE size)
{
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size.cx;
    bmi.bmiHeader.biHeight = -size.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    return bmi;
}

//
// PNG LOADER
//

HBITMAP renderTextToAlphaBitmap(const HFONT font, const std::wstring& text);

struct IWICImagingFactory;
class PNGLoader {
    IWICImagingFactory* pFactory = nullptr;

    PNGLoader();
    ~PNGLoader();

public:
    HBITMAP getBitmapFromPng(const std::wstring& pngPath, int* customIconSize);

    static PNGLoader& get()
    {
        static PNGLoader instance;
        return instance;
    }
};

//
// FILE
//

class FileManager {
    std::wstring _iniPath;
    FileManager();

public:
    static FileManager& get()
    {
        static FileManager instance;
        return instance;
    }

    void loadWindowRect(RECT& rect) const;
    void saveWindowRect(const RECT& rect) const;
};