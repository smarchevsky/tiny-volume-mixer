#pragma once

#include "Common.h"

#include "Draw.h"

#include <string>
#include <unordered_map>
#include <vector>

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

//
// PNG LOADER
//

class IWICImagingFactory;
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