#pragma once
#include "Common.h"

#include <string>
#include <unordered_map>

struct UIConfig {
    DWORD windowBackgroundRGBA;
    DWORD windowBorderRGBA;
    DWORD sliderDefaultBackgroundRGB;
    DWORD sliderDefaultBorderRGB;

    BYTE windowBorderWidth;
    BYTE windowCornerRadius;

    BYTE sliderWidthApp;
    BYTE sliderWidthMaster;
    BYTE sliderSpacing;
    BYTE sliderCornerRadius;
    BYTE sliderBorderWidth;

    BYTE fontSize;
    BYTE iconSize;

    int getSliderOffsetL() const { return sliderSpacing / 2; }
    int getSliderOffsetR() const { return sliderSpacing - getSliderOffsetL(); }

    UIConfig();
};

struct SliderInfo {
    HBITMAP textBmp;
    HICON hIconLarge;
    uint64_t iconHash;
    DWORD colorSlider;
    DWORD colorSecondary;
    bool colorsInitialized;
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
