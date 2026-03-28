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

class PixelBuffer_RG {
    BYTE* buf {}; // w, h, offset1, data0, data1
    static constexpr int sizeSize = sizeof(LONG) * 3;
    inline LONG getIntData(int index) const { return *((LONG*)buf + index); }
    inline LONG& getIntData(int index) { return *((LONG*)buf + index); }

public:
    SIZE size() const { return { getIntData(0), getIntData(1) }; }
    template <int ChannelIndex>
    const BYTE* data() const
    {
        static_assert(ChannelIndex == 0 || ChannelIndex == 1);
        if (!buf)
            return nullptr;
        if constexpr (ChannelIndex == 0)
            return buf + sizeSize;
        else
            return buf + getIntData(2);
    }

    template <int ChannelIndex>
    BYTE* data() { return const_cast<BYTE*>(const_cast<const PixelBuffer_RG*>(this)->data<ChannelIndex>()); }

    void allocateSize(int w, int h)
    {
        int singleChannelSize = w * h;
        buf = (BYTE*)malloc(sizeSize + singleChannelSize * 2);
        getIntData(0) = w, getIntData(1) = h;
        getIntData(2) = sizeSize + singleChannelSize;
    }

    void cleanup()
    {
        if (buf)
            free(buf);
        buf = nullptr;
    }
};

class TextRenderer {
    HFONT _hFont {};

    TextRenderer() = default;
    ~TextRenderer();

public:
    PixelBuffer_RG renderTextToGrayscaleStencil(const std::wstring& text);

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