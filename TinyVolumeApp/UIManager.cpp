#define NOMINMAX

#include "UIManager.h"

#include "ColorUtils.h"
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

// GetFileVersionInfoW, VerQueryValueW
#include <winver.h>
#pragma comment(lib, "Version.lib")

#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;
#define CLAMP_ICON_SIZE(size) std::clamp(size, 8, 256)

namespace {

template <class T>
inline void hash_combine(uint64_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

SliderInfo createIconInfo(HICON icon, bool calculateIconColor = true)
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
    BITMAPINFO bmi = getBMI_ARGB({ bmp.bmWidth, bmp.bmHeight });
    GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, &pixels[0], (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    // make icon info
    SliderInfo ii {};

    ii.hIconLarge = icon;

    ii.colorSlider = defaultSliderColors.first;
    ii.colorSecondary = defaultSliderColors.second;

    if (calculateIconColor)
        ColorUtils::calculatePriorityColor(&pixels[0],
            bmp.bmWidth, bmp.bmHeight, ii.colorSlider, ii.colorSecondary);

    uint64_t iconHash {};
    for (int i = 0; i < pixelCount; ++i)
        hash_combine(iconHash, pixels[i]);
    ii.iconHash = iconHash;

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
    SIZE iconSize;
    getBitmapData(hBmp, iconSize, pixels);
    HBITMAP hMask = CreateBitmap(iconSize.cx, iconSize.cy, 1, 1, nullptr);
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

std::wstring getFileName(const LPWSTR path)
{
    std::wstring result;
    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeW(path, &dummy);
    if (size > 0) {
        result.resize(size);
        if (GetFileVersionInfoW(path, 0, size, (void*)result.c_str())) {
            LPWSTR description = {};
            UINT descLen = 0;
            if (VerQueryValueW(result.c_str(), L"\\StringFileInfo\\040904b0\\FileDescription",
                    (LPVOID*)&description, &descLen)) {
                wprintf(L"DESCRIPTION: %s\n", description);
                result = description;
                return result;
            }
        }
    }

    wchar_t exePath[MAX_PATH];
    if (SUCCEEDED(SHLoadIndirectString(path, exePath, MAX_PATH, NULL))) {
        if (LPWSTR fileName = PathFindFileNameW(exePath)) {
            if (fileName[0] >= L'a' && fileName[0] <= L'z')
                fileName[0] += L'A' - L'a';
            // if (WCHAR* extension = wcsrchr(fileName, L'.'))
            if (LPWSTR extension = wcsstr(fileName, L".exe"))
                *extension = '\0';
            wprintf(L"FILE NAME: %s\n", fileName);
            result = fileName;
            return result;
        }
    }

    return result;
}
} // namespace

#pragma region ICON
SliderInfo* UIManager::generateSliderInfo(WCHAR* iconPath, PID pid)
{
    HBITMAP textBmp {};
    std::wstring iconPathStr;
    if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) {
        wchar_t exePath[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
            iconPathStr = exePath;
            auto fileName = getFileName(exePath);
            if (pid != 0 && !fileName.empty()) {
                textBmp = renderTextToAlphaBitmap(fileName);
            }
        }

        CloseHandle(hProcess);
    }

    if (pid == 0)
        textBmp = renderTextToAlphaBitmap(L"System");

    if (iconPath && wcslen(iconPath)) { // retrieve from AudioSessionInfo
        iconPathStr = iconPath;
    }

    if (iconPathStr.empty())
        return nullptr;

    HICON icon = createIconFromPath(iconPathStr, _iconSize);
    if (!icon)
        icon = createIconFromPackageInstallPath(pid, _iconSize);
    if (!icon)
        icon = _iiNoIconApp.hIconLarge;

    auto& sliderInfo = _cachedAppIcons[iconPathStr] = createIconInfo(icon);
    sliderInfo.textBmp = textBmp;
    sliderInfo.iconHash = pid == 0 ? uint64_t(-1) : sliderInfo.iconHash; // sort system sounds first/last
    return &sliderInfo;
}

__forceinline int recipCurve(int x)
{
    x = 255 - x, x = x * x / 255;
    return 255 - x;
}

HBITMAP UIManager::renderTextToAlphaBitmap(const std::wstring& text)
{
    const int bkgStripExtend = 30;
    const float bkgGradientWidth = 80.f;
    const float bkgStripMax = 80;

    HDC fontBufferDC = CreateCompatibleDC(NULL);

    HFONT hOldFont = (HFONT)SelectObject(fontBufferDC, _hFont);

    SIZE textSize;
    GetTextExtentPoint32(fontBufferDC, text.c_str(), (int)text.length(), &textSize);
    textSize.cx += bkgStripExtend * 2;
    textSize.cy += 2;

    DWORD* pixelsARGB;
    BITMAPINFO bmi = getBMI_ARGB(textSize);
    HBITMAP fontBufferBitmap = CreateDIBSection(fontBufferDC, &bmi, DIB_RGB_COLORS, (void**)&pixelsARGB, NULL, 0);

    if (fontBufferBitmap) {
        HBITMAP hOldBmp = (HBITMAP)SelectObject(fontBufferDC, fontBufferBitmap);
        SetBkMode(fontBufferDC, TRANSPARENT);
        SetTextColor(fontBufferDC, RGB(255, 255, 255)); // White text
        TextOut(fontBufferDC, bkgStripExtend, 0, text.c_str(), (int)text.length());

        float deriv = (float)bkgStripMax / bkgGradientWidth;
        for (int y = 0; y < textSize.cy; ++y) {
            for (int x = 0; x < textSize.cx; ++x) {
                int i = y * textSize.cx + x;
                ARGB_SPLIT(BYTE, , pixelsARGB[i]);

                a = (BYTE)std::min((textSize.cx - x - 1) * deriv,
                    std::min(x * deriv, bkgStripMax));

                a = std::max(a, r);
                r = recipCurve(r);
                pixelsARGB[i] = ARGB(a, r, r, r);
            }
        }
        SelectObject(fontBufferDC, hOldBmp);
    }

    SelectObject(fontBufferDC, hOldFont);
    DeleteDC(fontBufferDC);

    return fontBufferBitmap;
}

void UIManager::init(const UIConfig& config)
{
    uninit();

    // TEXT

    _hFont = CreateFont(
        std::clamp((int)config.fontSize, 8, 255), // Height (arbitrary size)
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

    // ICONS

    _iconSize = config.iconSize;

    wchar_t dllPathSource[MAX_PATH];
    GetSystemDirectoryW(dllPathSource, MAX_PATH);

    auto loadIcon = [dllPathSource, this](const wchar_t* path, int iconIndex) -> SliderInfo {
        wchar_t dllPath[MAX_PATH];
        wcscpy_s(dllPath, MAX_PATH, dllPathSource);
        wcscat_s(dllPath, path);

        HICON icon {};
        SHDefExtractIconW(dllPath, iconIndex, 0, &icon, NULL, MAKELONG(CLAMP_ICON_SIZE(_iconSize), 0));
        auto sliderInfo = createIconInfo(icon, false);
        sliderInfo.textBmp = renderTextToAlphaBitmap(L"Master");
        return sliderInfo;
    };

    _iiMasterSpeaker = loadIcon(L"\\mmres.dll", -3004);
    _iiMasterHeadphones = loadIcon(L"\\mmres.dll", -3015);
    _iiNoIconApp = loadIcon(L"\\imageres.dll", -15);
}

void UIManager::uninit()
{
    for (auto& pair : _cachedAppIcons) {
        auto& iconInfo = pair.second;
        DestroyIcon(iconInfo.hIconLarge);
        DeleteObject(iconInfo.textBmp);
        iconInfo = {};
    }

    DestroyIcon(_iiMasterSpeaker.hIconLarge);
    DestroyIcon(_iiMasterHeadphones.hIconLarge);
    DestroyIcon(_iiNoIconApp.hIconLarge);
    _iiMasterSpeaker = _iiMasterHeadphones = _iiNoIconApp = {};

    if (_hFont) {
        DeleteObject(_hFont);
        _hFont = nullptr;
    }
}

#pragma endregion