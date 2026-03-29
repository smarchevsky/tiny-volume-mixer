#include "Utils.h"

// SHLoadIndirectString, PathFindFileNameW, SHCreateStreamOnFileW, StrStrW
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

// everything related to PNG loader
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

// SHDefExtractIconW, SHGetKnownFolderPath
#include <shlobj.h>

#include <algorithm>
#include <cassert>
#include <string>

//
// PNG LOADER
//

PNGLoader::PNGLoader() { CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory)); }
PNGLoader::~PNGLoader() { pFactory->Release(); }
HBITMAP PNGLoader::getBitmapFromPng(const std::wstring& pngPath, int* customIconSize)
{
    IWICBitmapDecoder* pDecoder = nullptr;
    pFactory->CreateDecoderFromFilename(pngPath.c_str(), nullptr,
        GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);

    IWICBitmapFrameDecode* pFrame = nullptr;
    pDecoder->GetFrame(0, &pFrame);

    IWICBitmapScaler* pScaler = nullptr;
    if (customIconSize) {
        pFactory->CreateBitmapScaler(&pScaler);
        pScaler->Initialize(pFrame, *customIconSize, *customIconSize, WICBitmapInterpolationModeHighQualityCubic);
    }

    IWICFormatConverter* pConverter = nullptr;
    pFactory->CreateFormatConverter(&pConverter);

    WICPixelFormatGUID srcFormat;
    pFrame->GetPixelFormat(&srcFormat);

    pConverter->Initialize(pScaler ? pScaler : (IWICBitmapSource*)pFrame, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

    UINT w = 0, h = 0;
    pConverter->GetSize(&w, &h);

    void* pBits = nullptr;

    HDC hdc = GetDC(nullptr);
    BITMAPINFO bmi = getBMI_ARGB({ (LONG)w, (LONG)h });
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ReleaseDC(nullptr, hdc);

    pConverter->CopyPixels(nullptr, w * 4, w * h * 4, (BYTE*)pBits);

    pConverter->Release();
    if (pScaler)
        pScaler->Release();
    pFrame->Release();
    pDecoder->Release();
    return hBmp;
}

//
// FONT
//

//
// FILE
//

#pragma region FILE
FileManager::FileManager()
{
    PWSTR appDataPath = NULL;
    _iniPath = L"";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
        _iniPath = std::wstring(appDataPath) + L"\\VolumeApp.ini";
        CoTaskMemFree(appDataPath);
    }
}

constexpr int defaultX = 100, defaultY = 100, defaultWidth = 600, defaultHeight = 300;
void FileManager::loadWindowRect(RECT& rect) const
{
    std::wstring iniPath = _iniPath;
    if (iniPath.empty())
        return;

    rect.left = GetPrivateProfileIntW(L"Window", L"x", 100, iniPath.c_str());
    rect.top = GetPrivateProfileIntW(L"Window", L"y", 100, iniPath.c_str());
    int width = GetPrivateProfileIntW(L"Window", L"w", 800, iniPath.c_str());
    int height = GetPrivateProfileIntW(L"Window", L"h", 600, iniPath.c_str());
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;

    /* HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
    if (hMonitor == NULL) {
        rect.left = defaultX;
        rect.top = defaultY;

    } else {
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfo(hMonitor, &mi)) {
            if (rect.right > mi.rcWork.right)
                rect.left = mi.rcWork.right - width;
            if (rect.bottom > mi.rcWork.bottom)
                rect.top = mi.rcWork.bottom - height;
            if (rect.left < mi.rcWork.left)
                rect.left = mi.rcWork.left;
            if (rect.top < mi.rcWork.top)
                rect.top = mi.rcWork.top;
        }
    } */
}

void FileManager::saveWindowRect(const RECT& rect) const
{
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    WritePrivateProfileStringW(L"Window", L"x", std::to_wstring(rect.left).c_str(), _iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"y", std::to_wstring(rect.top).c_str(), _iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"w", std::to_wstring(width).c_str(), _iniPath.c_str());
    WritePrivateProfileStringW(L"Window", L"h", std::to_wstring(height).c_str(), _iniPath.c_str());
}
#pragma endregion

HBITMAP renderTextToAlphaBitmap(const HFONT font, const std::wstring& text)
{
    assert(font);
    HDC fontBufferDC = CreateCompatibleDC(NULL);

    HFONT hOldFont = (HFONT)SelectObject(fontBufferDC, font);

    SIZE textSize;
    GetTextExtentPoint32(fontBufferDC, text.c_str(), (int)text.length(), &textSize);

    DWORD* pixelsARGB;
    BITMAPINFO bmi = getBMI_ARGB(textSize);
    HBITMAP fontBufferBitmap = CreateDIBSection(fontBufferDC, &bmi, DIB_RGB_COLORS, (void**)&pixelsARGB, NULL, 0);

    if (fontBufferBitmap) {
        HBITMAP hOldBmp = (HBITMAP)SelectObject(fontBufferDC, fontBufferBitmap);
        SetBkMode(fontBufferDC, TRANSPARENT);
        SetTextColor(fontBufferDC, RGB(255, 255, 255)); // White text
        TextOut(fontBufferDC, 0, 0, text.c_str(), (int)text.length());
        for (int i = 0; i < textSize.cx * textSize.cy; ++i)
            pixelsARGB[i] = (pixelsARGB[i] & 0xFF) << 24;
        SelectObject(fontBufferDC, hOldBmp);
    }

    SelectObject(fontBufferDC, hOldFont);
    DeleteDC(fontBufferDC);

    return fontBufferBitmap;
}
