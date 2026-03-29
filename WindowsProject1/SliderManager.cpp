#include "SliderManager.h"

#include "Draw.h"
#include "IconManager.h"

#if defined(_DEBUG)

#define DEBUG_ITERATE_ICONS 0
#if DEBUG_ITERATE_ICONS == 1

// SHDefExtractIconW, SHGetKnownFolderPath
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
    auto path = L"C:\\Windows\\System32\\mmres.dll";
    if (iconIDs.empty())
        ListIconIDs(path);

    if (iconIDs.size()) {
        int iconIndex = iconIDs[currentIndex];
        SHDefExtractIconW(path, -iconIndex, 0, &icon, NULL, MAKELONG(iconSize, 0));

        wprintf(L"iconIndex: %d\n", iconIndex);
        currentIndex = (currentIndex + 1) % iconIDs.size();
    }
    return icon;
}
#endif
#endif

void Slider::draw(HDC hdc, const UIConfig& uic) const
{
#if DEBUG_ITERATE_ICONS == 1
    const_cast<IconInfo*>(_iconInfo)->hLarge = debugUpdateIcon(_iconInfo->width);
#endif

    float drawHeight = (_rect.bottom - _rect.top) * (1.f - _val);
    RECT drawRect {
        _rect.left + uic.getSliderOffsetL(), _rect.top + LONG(drawHeight),
        _rect.right - uic.getSliderOffsetR(), _rect.bottom
    };

    const DWORD border = _focused ? 0xFF000000 : 0xAA000000;
    DWORD sliderColor = _iconInfo ? _iconInfo->ARGB : defaultSliderColor;

    drawBorderedRect(hdc, drawRect,
        uic.sliderCornerRadius, uic.sliderBorderWidth,
        0xAA000000 | sliderColor, border | sliderColor);

    if (_iconInfo && _iconInfo->textBmp && _focused)
        drawStencil(hdc, _iconInfo->textBmp, { drawRect.left, _rect.top }, drawRect, uic.sliderCornerRadius, 0xFFFFFF, 0xFFFFFF);

    if (_iconInfo && _iconInfo->hLarge)
        DrawIconEx(hdc,
            (_rect.left + _rect.right) / 2 - uic.iconSize / 2,
            _rect.bottom - uic.iconSize / 2 - (uic.sliderWidthApp - uic.sliderSpacing) / 2,
            _iconInfo->hLarge, 0, 0, 0, NULL, DI_NORMAL);
}

Slider* SliderManager::getSliderFromSelect(SelectInfo info)
{
    if (info._type == VolumeType::Master)
        return &_sliderMaster;

    else if (info._type == VolumeType::App) {
        auto it = std::find_if(_slidersApps.begin(), _slidersApps.end(), [&](const Slider& s) { return s.getPID() == info._pid; });
        if (it != _slidersApps.end())
            return &*it;
    }

    return nullptr;
}

void SliderManager::appSliderAdd(PID pid, float vol, bool muted, const IconInfo* iconInfo)
{
    _slidersApps.push_back(Slider(pid, vol, iconInfo));
}

void SliderManager::appSliderRemove(PID pid)
{
    auto it = std::find_if(_slidersApps.begin(), _slidersApps.end(), [&](const Slider& s) { return s.getPID() == pid; });
    _slidersApps.erase(it);
}

SelectInfo SliderManager::getSelectAtPosition(POINT mousePos)
{
    if (_sliderMaster.intersects(mousePos)) {
        return SelectInfo(VolumeType::Master, (PID)0);
    }

    for (int i = 0; i < _slidersApps.size(); ++i)
        if (_slidersApps.at(i).intersects(mousePos)) {
            return SelectInfo(VolumeType::App, _slidersApps.at(i).getPID());
        }

    return {};
}

void SliderManager::recalculateSliderRects(const RECT& r, const UIConfig& uic)
{
    int offset = r.left + uic.getSliderOffsetR() + uic.frameBorderWidth;
    auto top = r.top + uic.sliderSpacing + uic.frameBorderWidth;
    auto bottom = r.bottom - uic.sliderSpacing - uic.frameBorderWidth;

    _sliderMaster._rect = { offset, top, offset += uic.sliderWidthMaster, bottom };

    for (int i = 1; i < _slidersApps.size(); ++i) {
        if (_slidersApps[i].getPID() == 0) {
            std::swap(_slidersApps[i], _slidersApps[0]);
            std::swap(_slidersApps[i]._focused, _slidersApps[0]._focused);
        }
    }

    for (auto& slider : _slidersApps) {
        slider._rect = { offset, top, offset += uic.sliderWidthApp, bottom };
    }
}

void SliderManager::drawSliders(HDC hdc, const UIConfig& uic)
{
    _sliderMaster.draw(hdc, uic);
    for (auto& slider : _slidersApps)
        slider.draw(hdc, uic);
}
