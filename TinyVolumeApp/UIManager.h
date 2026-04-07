#pragma once
#include "Common.h"

#include <string>
#include <unordered_map>

constexpr std::pair<DWORD, DWORD> defaultSliderColors = { 0x00888888, 0x00AAAAAA };

struct UIConfig {
    DWORD colorWindowFrame;
    DWORD colorWindowBck;
    uint8_t frameBorderWidth;
    uint8_t sliderSpacing;
    uint8_t sliderCornerRadius;
    uint8_t sliderWidthApp;
    uint8_t sliderWidthMaster;
    uint8_t sliderBorderWidth;
    uint8_t frameCornerRadius;
    uint8_t iconSize;
    uint8_t fontSize;

    int getSliderOffsetL() const { return sliderSpacing / 2; }
    int getSliderOffsetR() const { return sliderSpacing - getSliderOffsetL(); }

    UIConfig();
};

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

    HBITMAP renderTextToAlphaBitmap(const std::wstring& text);

    void init(const UIConfig& config);
    static UIManager& get()
    {
        static UIManager instance;
        return instance;
    }
};
