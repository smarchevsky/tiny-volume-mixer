#include "Utils.h"

// SHLoadIndirectString, PathFindFileNameW, SHCreateStreamOnFileW, StrStrW
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

// everything related to PNG loader
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

// SHDefExtractIconW, SHGetKnownFolderPath
#include <shlobj.h>

//
// PNG LOADER
//

namespace {
struct PngLoaderData {
    IWICBitmapDecoder* pDecoder {};
    IWICBitmapFrameDecode* pFrame {};
    IWICBitmapScaler* pScaler {};
    IWICFormatConverter* pConverter {};
    WICPixelFormatGUID srcFormat {};

    // decoder already initialized
    void initInternal(IWICImagingFactory* pFactory, int* customImageSize, bool grayscale)
    {
        pDecoder->GetFrame(0, &pFrame);

        if (customImageSize) {
            pFactory->CreateBitmapScaler(&pScaler);
            pScaler->Initialize(pFrame, *customImageSize, *customImageSize,
                WICBitmapInterpolationModeHighQualityCubic);
        }

        pFactory->CreateFormatConverter(&pConverter);
        pFrame->GetPixelFormat(&srcFormat);

        pConverter->Initialize(pScaler ? pScaler : (IWICBitmapSource*)pFrame,
            grayscale
                ? GUID_WICPixelFormat8bppGray
                : GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    }

    PngLoaderData(IWICImagingFactory* pFactory, const std::wstring& pngPath, int* customImageSize, bool grayscale)
    {
        pFactory->CreateDecoderFromFilename(pngPath.c_str(), nullptr,
            GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
        initInternal(pFactory, customImageSize, grayscale);
    }

    PngLoaderData(IWICImagingFactory* pFactory, int resourceID, int* customImageSize, bool grayscale)
    {
        HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceID), L"PNG");
        if (hRes) {
            HGLOBAL hData = LoadResource(NULL, hRes);
            void* pMem = LockResource(hData);
            DWORD size = SizeofResource(NULL, hRes);

            if (IStream* pStream = SHCreateMemStream((BYTE*)pMem, size)) {
                if (SUCCEEDED(pFactory->CreateDecoderFromStream(pStream, NULL,
                        WICDecodeMetadataCacheOnLoad, &pDecoder))) {
                    initInternal(pFactory, customImageSize, grayscale);
                    pStream->Release();
                }
            }
        }
    }

    ~PngLoaderData()
    {
        pConverter->Release();
        if (pScaler)
            pScaler->Release();
        pFrame->Release();
        pDecoder->Release();
    }
};
} // namespace

PNGLoader::PNGLoader() { CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_pFactory)); }
PNGLoader::~PNGLoader() { _pFactory->Release(); }

HBITMAP PNGLoader::getBitmapFromPng(const std::wstring& pngPath, int* customImageSize)
{
    PngLoaderData plData(_pFactory, pngPath, customImageSize, false);

    UINT w = 0, h = 0;
    plData.pConverter->GetSize(&w, &h);

    void* pBits = nullptr;
    BITMAPINFO bmi = getBMI_ARGB({ (LONG)w, (LONG)h });
    HDC hdc = GetDC(nullptr);
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    ReleaseDC(nullptr, hdc);

    constexpr UINT numChannels = 4;
    plData.pConverter->CopyPixels(nullptr, w * numChannels, w * h * numChannels, (BYTE*)pBits);

    return hBmp;
}

ImageBuffer8 PNGLoader::getGrayscalePngFromResource(int resourceID, int* customImageSize)
{
    PngLoaderData plData(_pFactory, resourceID, customImageSize, true);

    UINT w = 0, h = 0;
    plData.pConverter->GetSize(&w, &h);
    ImageBuffer8 imageBuffer { .w = (LONG)w, .h = (LONG)h, .data = new BYTE[w * h] };
    plData.pConverter->CopyPixels(nullptr, w, w * h, imageBuffer.data);

    return imageBuffer;
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
