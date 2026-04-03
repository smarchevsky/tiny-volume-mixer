#pragma once
#include "Common.h"

#include <string>
#include <unordered_map>

constexpr std::pair<DWORD, DWORD> defaultSliderColors = { 0x00888888, 0x00AAAAAA };

struct SliderInfo {
    HBITMAP textBmp;
    HICON hIconLarge;
    uint64_t iconHash;
    std::pair<DWORD, DWORD> sliderColors;
    DWORD colorSlider;
    DWORD colorSecondary;
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
