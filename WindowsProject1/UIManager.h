#pragma once
#include "Common.h"

#include <string>
#include <unordered_map>

constexpr DWORD defaultSliderColor = 0x00AAAAAA;

struct SliderInfo {
    HBITMAP textBmp;
    HICON hIconLarge;
    DWORD ARGB;
};

class UIManager {
    std::unordered_map<std::wstring, SliderInfo> _cachedAppIcons;
    SliderInfo _iiMasterSpeaker {}, _iiMasterHeadphones {}, _iiNoIconApp {};
    int _iconSize = 24;

    HFONT _hFont {};

private:
    UIManager() = default;
    ~UIManager() { uninit(); }
    void uninit();

public:
    const SliderInfo* getIconMasterVol() const { return &_iiMasterSpeaker; }
    SliderInfo* generateSliderInfo(WCHAR* iconPath, PID pid);

    void init(const UIConfig& config);
    static UIManager& get()
    {
        static UIManager instance;
        return instance;
    }
};
