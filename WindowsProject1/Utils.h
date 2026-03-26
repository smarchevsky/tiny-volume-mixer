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
// ICON
//

struct IconInfo {
    HICON hLarge;
    DWORD ARGB;
    int width;
};

class IconManager {
    IconManager() = default;
    IconManager(const IconManager&) = delete;
    int _iconSize = 24;

    std::unordered_map<std::wstring, IconInfo> _cachedAppIcons;
    IconInfo _iiMasterSpeaker {}, _iiMasterHeadphones {}, _iiNoIconApp {};

public:
    void init(int iconSize);
    void uninit();

    IconInfo* getIconMasterVol() { return &_iiMasterSpeaker; }
    IconInfo* tryRetrieveIcon(WCHAR* iconPath, PID pid);

    static IconManager& get()
    {
        static IconManager instance;
        return instance;
    }
};

//
// SLIDER
//

static bool isValidRect(const RECT& rect) { return rect.right > rect.left && rect.bottom > rect.top; }

class Slider {
    PID _pid;

public:
    const IconInfo* _iconInfo {};
    RECT _rect;
    float _val;
    bool _focused;

    Slider(PID pid, float value, const IconInfo* iconInfo)
    {
        _pid = pid, _iconInfo = iconInfo, _rect = { 0 }, _val = value, _focused = false;
    }
    Slider() = default;

    PID getPID() const { return _pid; }
    float getHeight() const { return float(_rect.bottom - _rect.top); }
    bool intersects(POINT pos) const { return isValidRect(_rect) ? PtInRect(&_rect, pos) : false; }
    void draw(HDC hdc, const UIConfig& uic) const;

#if defined(_DEBUG)
    void debugUpdateIcon(int iconSize);
#endif
};

//
// SLIDER MANAGER
//

class SliderManager {
    Slider _sliderMaster {};
    std::vector<Slider> _slidersApps;

public:
    void appSliderAdd(PID pid, float vol, bool muted, const IconInfo* iconInfo);
    void appSliderRemove(PID pid);

    Slider* getSliderFromSelect(SelectInfo info);
    SelectInfo getSelectAtPosition(POINT mousePos);

    void recalculateSliderRects(const RECT& rect, const UIConfig& uic);
    void drawSliders(HDC hdc, const UIConfig& uic);
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