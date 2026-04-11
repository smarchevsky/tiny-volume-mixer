#define NOMINMAX

#include "SliderManager.h"

#include "ColorUtils.h"
#include "Draw.h"
#include "UIManager.h"
#include "Utils.h"

#include <algorithm> // std::sort

#if defined(_DEBUG)

#define DEBUG_ITERATE_ICONS 0
#if DEBUG_ITERATE_ICONS == 1

// SHDefExtractIconW, SHGetKnownFolderPath
#include "ColorUtils.h"
#include <shlobj.h>

// Callback function that Windows calls for every resource found
BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
{
    auto* ids = reinterpret_cast<std::vector<int>*>(lParam);
    if (IS_INTRESOURCE(lpszName))
        ids->push_back((int)(size_t)lpszName);
    else
        wprintf(L"Named Resource Found: %s\n", lpszName);
    return TRUE; // Continue enumeration
}

static std::vector<int> iconIDs;
static int currentIndex;
void ListIconIDs(const std::wstring& dllPath)
{
    HMODULE hMod = LoadLibraryExW(dllPath.c_str(), NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!hMod) {
        wprintf(L"Failed to load: %s\n", dllPath.c_str());
        return;
    }
    wprintf(L"Listing Icon IDs for: %s\n", dllPath.c_str());
    EnumResourceNamesW(hMod, RT_GROUP_ICON, EnumResNameProc, reinterpret_cast<LONG_PTR>(&iconIDs));
    for (int id : iconIDs)
        wprintf(L"Found ID: %d\n", id);
    FreeLibrary(hMod);
}

HICON debugUpdateIcon(int iconSize)
{
    HICON icon {};
    // imageres, mmres, shell32, setupapi
    auto path = L"C:\\Windows\\System32\\setupapi.dll";
    if (iconIDs.empty())
        ListIconIDs(path);

    if (iconIDs.size()) {
        int iconIndex = iconIDs[currentIndex / 3];
        SHDefExtractIconW(path, -iconIndex, 0, &icon, NULL, MAKELONG(iconSize, 0));

        wprintf(L"iconIndex: %d\n", iconIndex);
        currentIndex = (currentIndex + 1) % (iconIDs.size() * 3);
    }
    return icon;
}
#endif
#endif

inline RECT operator-(RECT rc, POINT p)
{
    rc.left -= p.x, rc.top -= p.y, rc.right -= p.x, rc.bottom -= p.y;
    return rc;
}

inline RECT operator+(RECT rc, POINT p)
{
    rc.left += p.x, rc.top += p.y, rc.right += p.x, rc.bottom += p.y;
    return rc;
}

void Slider::draw(HDC hdc, const UIConfig& uic) const
{
#if DEBUG_ITERATE_ICONS == 1
    SliderInfo& mutThis = const_cast<SliderInfo&>(*_sliderInfo);
    mutThis.hIconLarge = debugUpdateIcon(uic.iconSize);

    ICONINFO iconInfo;
    GetIconInfo(mutThis.hIconLarge, &iconInfo);

    BITMAP bmp;
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bmp);

    int pixelCount = bmp.bmWidth * bmp.bmHeight;
    if (pixelCount <= 256 * 256) {
        std::vector<DWORD> pixels(pixelCount);
        HDC hdc = GetDC(NULL);
        BITMAPINFO bmi = getBMI_ARGB({ bmp.bmWidth, bmp.bmHeight });
        GetDIBits(hdc, iconInfo.hbmColor, 0, bmp.bmHeight, &pixels[0], (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
        ReleaseDC(NULL, hdc);

        ColorUtils::calculatePriorityColor(&pixels[0],
            bmp.bmWidth, bmp.bmHeight, mutThis.colorSlider, mutThis.colorSecondary);
    }

#endif

#define OVERWRITE 1

    float drawHeight = (_rect.bottom - _rect.top) * (1.f - _val);
    const RECT roundRect {
        _rect.left + uic.getSliderOffsetL(), _rect.top + LONG(drawHeight),
        _rect.right - uic.getSliderOffsetR(), _rect.bottom
    };

    const DWORD borderAlphaMask = _focused ? 0xFF000000 : 0xAA000000;
    bool colorsValid = _sliderInfo && _sliderInfo->colorsInitialized;

    DWORD sliderColor = colorsValid ? _sliderInfo->colorSlider : uic.sliderDefaultBackgroundRGB;
    DWORD sliderColor2 = colorsValid ? _sliderInfo->colorSecondary : uic.sliderDefaultBorderRGB;

    // VOL
    DWORD col_bg = 0x66000000 | sliderColor;
    DWORD col_bo = borderAlphaMask | sliderColor2;

#if OVERWRITE == 1
    DWORD bg = compositeAlpha(0, uic.windowBackgroundRGBA);
    col_bg = compositeAlpha(bg, col_bg);
    col_bo = compositeAlpha(bg, col_bo);
    drawBorderedRectOverwrite(hdc, roundRect, uic.sliderCornerRadius, uic.sliderBorderWidth,
        col_bg, col_bo);
#else
    drawBorderedRectAlphaComposite(hdc, roundRect, uic.sliderCornerRadius, uic.sliderBorderWidth,
        bg_col, col_bo);
#endif
    // PEAK
    float peak = _peak;
    peak = peak * (2 - peak);
    RECT peakRect { roundRect.left + 6, roundRect.top + 6, roundRect.right - 6, roundRect.bottom - 6 };
    peakRect.top += LONG((peakRect.bottom - peakRect.top) * (1.f - peak));

    DWORD blendedPeakBkg = compositeAlpha(col_bg, 0xFF000000 | sliderColor2);
#if OVERWRITE == 1
    drawBorderedRectOverwrite(hdc, peakRect, 3, 0, blendedPeakBkg, blendedPeakBkg);
#else
    drawBorderedRectAlphaComposite(hdc, peakRect, 3, 0, blendedPeakBkg, blendedPeakBkg);
#endif

    // ICON
    if (_sliderInfo && _sliderInfo->hIconLarge)
        DrawIconEx(hdc,
            (_rect.left + _rect.right) / 2 - uic.iconSize / 2,
            _rect.bottom - uic.iconSize / 2 - (uic.sliderWidthApp - uic.sliderSpacing) / 2,
            _sliderInfo->hIconLarge, 0, 0, 0, NULL, DI_NORMAL);
}

RECT Slider::calculateTextRect() const
{
    SIZE bitmapSize {};
    DWORD* pixels;
    getBitmapData(_sliderInfo->textBmp, bitmapSize, pixels);

    LONG extend = bitmapSize.cx - (_rect.right - _rect.left);
    LONG x = std::max(_rect.left - extend / 2, (LONG)0);
    return RECT { x, _rect.top, x + _rect.right - _rect.left + extend, _rect.top + bitmapSize.cy };
}

Slider* SliderManager::getSliderFromHitUID(HitUID hitUID)
{
    if (_sliderMaster._hitUID == hitUID)
        return &_sliderMaster;
    auto it = std::find_if(_slidersApps.begin(), _slidersApps.end(), [&](const Slider& s) { return s._hitUID == hitUID; });
    if (it != _slidersApps.end())
        return &*it;
    return nullptr;
}

Slider* SliderManager::getSliderAppByPID(PID pid)
{
    auto it = std::find_if(_slidersApps.begin(), _slidersApps.end(), [&](const Slider& s) { return s.getPID() == pid; });
    if (it != _slidersApps.end())
        return &*it;
    return nullptr;
}

void SliderManager::appSliderAdd(PID pid, float vol, bool muted, const SliderInfo* sliderInfo)
{
    _slidersApps.push_back(Slider(pid, vol, sliderInfo));
}

void SliderManager::appSliderRemove(PID pid)
{
    std::erase_if(_slidersApps, [&](const Slider& s) { return s.getPID() == pid; });
}

void SliderManager::recalculateSliderRects(HitDetector& hitDetector, const RECT& windowRect, const UIConfig& uic)
{
    LONG offset = windowRect.left + uic.getSliderOffsetR() + uic.windowBorderWidth;
    LONG top = windowRect.top + uic.sliderSpacing + uic.windowBorderWidth;
    LONG bottom = windowRect.bottom - uic.sliderSpacing - uic.windowBorderWidth;

    // sort sliders by icon hash
    std::sort(_slidersApps.begin(), _slidersApps.end(), [](const Slider& s0, const Slider& s1) {
        if (s0._sliderInfo && s1._sliderInfo)
            return s0._sliderInfo->iconHash < s1._sliderInfo->iconHash;
        return false;
    });

    hitDetector.removeGroup(HitGroupType::Slider);
    {
        RECT rc = RECT { offset, top, offset += uic.sliderWidthMaster, bottom };
        _sliderMaster._hitUID = hitDetector.addRect(rc, HitGroupType::Slider);
        _sliderMaster._rect = rc;
    }

    for (auto& slider : _slidersApps) {
        RECT rc = RECT { offset, top, offset += uic.sliderWidthApp, bottom };
        slider._hitUID = hitDetector.addRect(rc, HitGroupType::Slider);
        slider._rect = rc;
    }
}

void SliderManager::drawSliders(HDC hdc, const UIConfig& uic)
{
    _sliderMaster.draw(hdc, uic);
    for (auto& slider : _slidersApps)
        slider.draw(hdc, uic);
}
