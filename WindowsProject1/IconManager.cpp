#include "IconManager.h"

#include "Utils.h" // PNGLoader

// GetPackageFullName, GetPackageFamilyName, GetPackagePathByFullName
#include <appmodel.h>

#include <xmllite.h> // CreateXMLReader
#pragma comment(lib, "xmllite.lib")

// SHDefExtractIconW, SHGetKnownFolderPath
#include <shlobj.h>

// SHLoadIndirectString, PathFindFileNameW, SHCreateStreamOnFileW, StrStrW
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <filesystem>

namespace fs = std::filesystem;
#define CLAMP_ICON_SIZE(size) std::clamp(size, 8, 256)

namespace {
COLORREF getAvgColorARGB(int width, int height, DWORD* pixels)
{
    uint64_t totalR = 0, totalG = 0, totalB = 0;
    int opaquePixels = 0;

    const int pixelCount = width * height;
    for (int i = 0; i < pixelCount; i++) {
        BYTE a = (pixels[i] >> 24) & 0xFF, r = (pixels[i] >> 16) & 0xFF, g = (pixels[i] >> 8) & 0xFF, b = pixels[i] & 0xFF;
        if (a > 127)
            totalR += r, totalG += g, totalB += b, opaquePixels++;
    }

    if (opaquePixels > 0)
        return ARGB(0, BYTE(totalR / opaquePixels), BYTE(totalG / opaquePixels), BYTE(totalB / opaquePixels));

    return defaultSliderColor;
}

IconInfo createIconInfo(HICON icon, bool calculateIconColor = true)
{
    // calc avg color
    ICONINFO iconInfo;
    GetIconInfo(icon, &iconInfo);

    BITMAP bmp;
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);

    int pixelCount = bmp.bmWidth * bmp.bmHeight;
    if (pixelCount > 256 * 256)
        return {};

    std::vector<DWORD> pixels(pixelCount);

    HDC hdc = GetDC(NULL);
    BITMAPINFO bmi = getBMI_ARGB(bmp.bmWidth, bmp.bmHeight);
    GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, &pixels[0], (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    // make icon info
    IconInfo ii {};

    ii.hLarge = icon;
    ii.width = bmp.bmWidth;
    ii.ARGB = calculateIconColor ? getAvgColorARGB(bmp.bmWidth, bmp.bmHeight, &pixels[0]) : defaultSliderColor;
    return ii;
}

std::wstring GetPackageInstallPath(DWORD pid)
{
    //                            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess)
        return {};

    WCHAR packageFamilyName[PACKAGE_FAMILY_NAME_MAX_LENGTH] = {};
    UINT32 len = PACKAGE_FAMILY_NAME_MAX_LENGTH;

    // GetPackageFamilyName tells us if this is a packaged app
    if (GetPackageFamilyName(hProcess, &len, packageFamilyName) != ERROR_SUCCESS) {
        CloseHandle(hProcess);
        return {}; // not a packaged app, fall back to normal icon extraction
    }

    WCHAR packageFullName[PACKAGE_FULL_NAME_MAX_LENGTH] = {};
    len = PACKAGE_FULL_NAME_MAX_LENGTH;
    GetPackageFullName(hProcess, &len, packageFullName);
    CloseHandle(hProcess);

    // Get install path from full name
    UINT32 pathLen = MAX_PATH;
    WCHAR installPath[MAX_PATH] = {};
    GetPackagePathByFullName(packageFullName, &pathLen, installPath);

    return installPath;
}

// Find the best scaled asset that actually exists on disk
std::wstring ResolveBestAsset(const std::wstring& installPath, const std::wstring& rawLogoPath, int desiredIconSize, int& outFoundSize)
{
    outFoundSize = desiredIconSize;
    fs::path base = fs::path(installPath) / rawLogoPath;
    fs::path dir = base.parent_path();
    std::wstring stem = base.stem().wstring(); // "Square44x44Logo"
    std::wstring ext = base.extension().wstring(); // ".png"

    fs::path candidate;
    int sampleSizes[3] { desiredIconSize, desiredIconSize * 2, 256 };
    for (int i = 0; i < 3; ++i) {
        candidate = dir / (stem + L".targetsize-" + std::to_wstring(sampleSizes[i]) + ext);
        if (fs::exists(candidate)) {
            outFoundSize = sampleSizes[i];
            return candidate.wstring();
        }
    }

    if (fs::exists(base))
        return base.wstring();

    return {}; // not found
}

// Parse AppxManifest.xml and return the logo PNG path
std::wstring GetLogoPathFromManifest(const std::wstring& installPath)
{
    std::wstring manifestPath = (fs::path(installPath) / L"AppxManifest.xml").wstring();

    // Open the manifest file
    IStream* pStream = nullptr;
    HRESULT hr = SHCreateStreamOnFileW(manifestPath.c_str(), STGM_READ, &pStream);
    if (FAILED(hr))
        return {};

    // Create XML reader
    IXmlReader* pReader = nullptr;
    hr = CreateXmlReader(__uuidof(IXmlReader), (void**)&pReader, nullptr);
    if (FAILED(hr)) {
        pStream->Release();
        return {};
    }

    pReader->SetInput(pStream);

    std::wstring logoPath;
    XmlNodeType nodeType;

    while (S_OK == pReader->Read(&nodeType)) {
        if (nodeType != XmlNodeType_Element)
            continue;

        // Get element name
        const WCHAR* localName = nullptr;
        pReader->GetLocalName(&localName, nullptr);

        // Look for VisualElements (could be prefixed: uap:VisualElements)
        if (localName && (wcscmp(localName, L"VisualElements") == 0 || wcscmp(localName, L"DefaultTile") == 0)) {
            // Walk attributes
            if (S_OK == pReader->MoveToFirstAttribute()) {
                do {
                    const WCHAR* attrName = nullptr;
                    const WCHAR* attrValue = nullptr;
                    pReader->GetLocalName(&attrName, nullptr);
                    pReader->GetValue(&attrValue, nullptr);

                    // Square44x44Logo is the taskbar/small icon
                    if (attrName && wcscmp(attrName, L"Square44x44Logo") == 0) {
                        logoPath = attrValue;
                        break;
                    }
                } while (S_OK == pReader->MoveToNextAttribute());
            }
        }

        if (!logoPath.empty())
            break;
    }

    pReader->Release();
    pStream->Release();

    return logoPath;
}

HICON createIconFromPng(const std::wstring& pngPath, int* customIconSize)
{
    HBITMAP hBmp = PNGLoader::get().getBitmapFromPng(pngPath, customIconSize);
    DWORD* pixels;
    int width, height;
    getBitmapData(hBmp, width, height, pixels);
    HBITMAP hMask = CreateBitmap(width, height, 1, 1, nullptr);
    ICONINFO ii = { TRUE, 0, 0, hMask, hBmp };
    HICON hIcon = CreateIconIndirect(&ii);
    DeleteObject(hBmp);
    return hIcon;
}

HICON createIconFromPackageInstallPath(PID pid, int desiredIconSize)
{
    desiredIconSize = CLAMP_ICON_SIZE(desiredIconSize);

    std::wstring installPath = GetPackageInstallPath(pid); // from earlier
    std::wstring logoPath = GetLogoPathFromManifest(installPath);
    if (logoPath.empty())
        return {};

    int foundCustomSize;
    std::wstring pngPath = ResolveBestAsset(installPath, logoPath, desiredIconSize, foundCustomSize);
    if (pngPath.empty())
        return {};

    return createIconFromPng(pngPath, (foundCustomSize == desiredIconSize) ? nullptr : &desiredIconSize);
}

HICON createIconFromPath(std::wstring& path, int iconSize)
{
    if (path.empty())
        return {};

    const WCHAR* fullPath = path.c_str();
    WCHAR cleanPathBuf[MAX_PATH];
    WCHAR expandedPath[MAX_PATH];
    int iconIndex = 0;

    if (fullPath[0] == L'@') { // handle  @%SystemRoot%\System32\AudioSrv.Dll,-203
        fullPath += 1;
        wcscpy_s(cleanPathBuf, MAX_PATH, fullPath);

        WCHAR* comma = wcsrchr(cleanPathBuf, L',');
        if (comma) {
            *comma = L'\0';
            iconIndex = _wtoi(comma + 1);
        }

        ExpandEnvironmentStringsW(cleanPathBuf, expandedPath, MAX_PATH);
        fullPath = expandedPath;
    }

    HICON icon {};
    SHDefExtractIconW(fullPath, iconIndex, 0, &icon, NULL, MAKELONG(CLAMP_ICON_SIZE(iconSize), 0));
    return icon;

    return {};
}

} // namespace

#pragma region ICON
IconInfo* IconManager::tryRetrieveIcon(WCHAR* iconPath, PID pid)
{
    std::wstring pathStr;

    if (iconPath && wcslen(iconPath)) { // retrieve from AudioSessionInfo
        pathStr = iconPath;

    } else { // retrieve from PID
        if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) {
            wchar_t exePath[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, exePath, &size))
                pathStr = exePath;
            CloseHandle(hProcess);
        }
    }

    if (pathStr.empty())
        return nullptr;

    HICON icon = createIconFromPath(pathStr, _iconSize);
    if (!icon)
        icon = createIconFromPackageInstallPath(pid, _iconSize);
    if (!icon)
        icon = _iiNoIconApp.hLarge;

    auto& cachedIcon = _cachedAppIcons[pathStr] = createIconInfo(icon);
    return &cachedIcon;
}

void IconManager::init(int iconSize)
{
    uninit();
    _iconSize = iconSize;

    wchar_t dllPathSource[MAX_PATH];
    GetSystemDirectoryW(dllPathSource, MAX_PATH);

    auto loadIcon = [dllPathSource, iconSize](const wchar_t* path, int iconIndex) -> IconInfo {
        wchar_t dllPath[MAX_PATH];
        wcscpy_s(dllPath, MAX_PATH, dllPathSource);
        wcscat_s(dllPath, path);

        HICON icon {};
        SHDefExtractIconW(dllPath, iconIndex, 0, &icon, NULL, MAKELONG(CLAMP_ICON_SIZE(iconSize), 0));
        return createIconInfo(icon, false);
    };

    _iiMasterSpeaker = loadIcon(L"\\mmres.dll", -3004);
    _iiMasterHeadphones = loadIcon(L"\\mmres.dll", -3015);
    _iiNoIconApp = loadIcon(L"\\imageres.dll", -15);
}

void IconManager::uninit()
{
    for (auto& pair : _cachedAppIcons) {
        auto& iconInfo = pair.second;
        if (iconInfo.hLarge) {
            DestroyIcon(iconInfo.hLarge);
            iconInfo.hLarge = nullptr;
        }
    }
    DestroyIcon(_iiMasterSpeaker.hLarge);
    DestroyIcon(_iiMasterHeadphones.hLarge);
    DestroyIcon(_iiNoIconApp.hLarge);
}

#pragma endregion