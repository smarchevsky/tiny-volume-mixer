#include "Utils.h"

#include "IconManager.h"
// #include <psapi.h> // GetModuleBaseName

// SHLoadIndirectString, PathFindFileNameW, SHCreateStreamOnFileW, StrStrW
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

// everything related to PNG loader
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

// GetFileVersionInfoW, VerQueryValueW
#include <winver.h>
#pragma comment(lib, "Version.lib")

// SHDefExtractIconW, SHGetKnownFolderPath
#include <shlobj.h>

#include <string>
// #include <vector>
#include <algorithm>
#include <cassert>

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
    BITMAPINFO bmi = getBMI_ARGB(w, h);
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

void printFileName(const WCHAR* path)
{
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeW(path, &dummy);
    if (size > 0) {
        std::vector<BYTE> data(size);
        if (GetFileVersionInfoW(path, 0, size, data.data())) {
            LPWSTR description = nullptr;
            UINT descLen = 0;
            if (VerQueryValueW(data.data(), L"\\StringFileInfo\\040904b0\\FileDescription",
                    (LPVOID*)&description, &descLen)) {
                wprintf(L"DESCRIPTION: %s\n", description);
                return;
            }
        }
    }

    wchar_t exePath[MAX_PATH];
    if (SUCCEEDED(SHLoadIndirectString(path, exePath, MAX_PATH, NULL))) {
        if (WCHAR* fileName = PathFindFileNameW(exePath)) {
            if (fileName[0] >= L'a' && fileName[0] <= L'z')
                fileName[0] += L'A' - L'a';
            // if (WCHAR* extension = wcsrchr(fileName, L'.'))
            if (WCHAR* extension = StrStrW(fileName, L".exe"))
                *extension = '\0';
            wprintf(L"FILE NAME: %s\n", fileName);
        }
    }
}

//
// FONT
//

TextRenderer::~TextRenderer()
{
    if (_hFont)
        DeleteObject(_hFont);
}

void TextRenderer::init(int fontSize)
{
    fontSize = std::clamp(fontSize, 8, 255);

    if (_hFont)
        DeleteObject(_hFont);

    _hFont = CreateFont(
        fontSize, // Height (arbitrary size)
        0, // Width (0 let's Windows choose best match)
        0, // Escapement
        0, // Orientation
        FW_BOLD, // Weight (e.g., FW_NORMAL, FW_BOLD)
        FALSE, // Italic
        FALSE, // Underline
        FALSE, // Strikeout
        ANSI_CHARSET, // Charset
        OUT_DEFAULT_PRECIS, // Out Precision
        CLIP_DEFAULT_PRECIS, // Clip Precision
        DEFAULT_QUALITY, // Quality
        DEFAULT_PITCH | FF_SWISS, // Pitch and Family
        L"Segoe UI" // Typeface name
    );

    // BitBlt(hdc, 194, 16, _textSize.cx, _textSize.cy, _fontDC, 0, 0, SRCCOPY);
}

StencilGrayscale TextRenderer::renderTextToGrayscaleStencil(const std::wstring& text)
{
    assert(_hFont);

    StencilGrayscale stencil;
    HDC fontBufferDC = CreateCompatibleDC(NULL);
    HFONT hOldFont = (HFONT)SelectObject(fontBufferDC, _hFont);

    SelectObject(fontBufferDC, _hFont);

    SIZE textSize;
    GetTextExtentPoint32(fontBufferDC, text.c_str(), (int)text.length(), &textSize);

    DWORD* pixelsARGB;
    BITMAPINFO bmi = getBMI_ARGB(textSize.cx, textSize.cy);
    HBITMAP fontBufferBitmap = CreateDIBSection(fontBufferDC, &bmi, DIB_RGB_COLORS, (void**)&pixelsARGB, NULL, 0);

    if (fontBufferBitmap) {
        SelectObject(fontBufferDC, fontBufferBitmap);

        // SetBkMode(fontBufferDC, TRANSPARENT);
        SetBkColor(fontBufferDC, RGB(0, 0, 0));
        SetTextColor(fontBufferDC, RGB(255, 255, 255));
        RECT rect = { 0, 0, textSize.cx, textSize.cy };
        TextOut(fontBufferDC, 0, 0, text.c_str(), (int)text.length());

        assert(textSize.cx >= 0);
        assert(textSize.cy >= 0);
        if (BYTE* buf = stencil.allocateSize(textSize.cx, textSize.cy)) {
            for (int y = 0; y < textSize.cy; ++y) {
                for (int x = 0; x < textSize.cx; ++x) {
                    int index = y * textSize.cx + x;
                    buf[index] = pixelsARGB[index] & 0xFF;
                }
            }
        }

        DeleteObject(fontBufferBitmap);
    }

    SelectObject(fontBufferDC, hOldFont);
    DeleteDC(fontBufferDC); 
    return stencil;
}

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
