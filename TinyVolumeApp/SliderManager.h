#pragma once

#include "Common.h"

#include <functional>
#include <vector>

struct SliderInfo;
struct UIConfig;

static bool isValidRect(const RECT& rect) { return rect.right > rect.left && rect.bottom > rect.top; }

class Slider {
    PID _pid {};

public:
    const SliderInfo* _sliderInfo {};
    RECT _rect {};
    float _val {};
    float _peak {};
    bool _focused {};

    Slider() = default;
    Slider(PID pid, float value, const SliderInfo* sliderInfo)
    {
        _pid = pid, _sliderInfo = sliderInfo, _val = value;
    }

    PID getPID() const { return _pid; }
    float getHeight() const { return float(_rect.bottom - _rect.top); }
    bool intersects(POINT pos) const { return isValidRect(_rect) ? PtInRect(&_rect, pos) : false; }
    void draw(HDC hdc, const UIConfig& uic) const;
    RECT calculateTextRect() const;
};

class SliderManager {
    Slider _sliderMaster {};
    std::vector<Slider> _slidersApps;

public:
    void appSliderAdd(PID pid, float vol, bool muted, const SliderInfo* sliderInfo);
    void appSliderRemove(PID pid);

    Slider* getSliderFromSelect(SelectInfo info);
    SelectInfo getSelectAtPosition(POINT mousePos);
    void forEachSliderApp(std::function<void(Slider&)> func)
    {
        for (auto& slider : _slidersApps)
            func(slider);
    }

    void recalculateSliderRects(const RECT& rect, const UIConfig& uic);
    void drawSliders(HDC hdc, const UIConfig& uic);
};
