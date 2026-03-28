#pragma once

#include "Common.h"

#include "Draw.h"

#include <string>

inline HBITMAP getBitmapFromHDC(HDC hdc) { return (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP); }
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

inline BITMAPINFO getBMI_ARGB(int w, int h)
{
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    return bmi;
}

//
// PNG LOADER
//

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

class TextRenderer {
    HFONT _hFont {};

    TextRenderer() = default;
    ~TextRenderer();

public:
    HBITMAP renderTextToAlphaBitmap(const std::wstring& text);

    void init(int fontSize = 28);
    static TextRenderer& get()
    {
        static TextRenderer instance;
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