#pragma once

#include "App.h"

#include "AudioUtils.h"
#include "HitDetector.h"
#include "SliderManager.h"
#include "UIManager.h"

class VolumeApp : public App {
    AudioUpdateListener _audioAppListerner;
    UIConfig _uic;
    SliderManager _sliderManager;
    HitDetector _hitDetector;
    HitUID _hitHovered = HitUID_invalid;
    bool _isAppHovered;

    virtual void onPaint(HDC hdc) override;
    virtual void onResize(RECT newRect) override;

    virtual void onMouseLeave() override;
    virtual void onMouseMove(POINT cursorClientPos, bool justEntered) override;
    virtual void onMouseScroll(POINT cursorClientPos, float delta) override;

public:
    void handleMMAppRegistered(AudioSessionInitInfo* sessionInitInfo);
    void handleMMAppUnegistered(WPARAM wParam, LPARAM lParam);
    void handleMMAppActivationChanged(WPARAM wParam, LPARAM lParam);
    void handleMMRefreshVol(WPARAM wParam, LPARAM lParam);
    void handleTimerUpdateUI();
    bool handleHoverChanged(bool isHovered);

    virtual void construct(HINSTANCE instance, WNDPROC wndProc);
    virtual void destroyWindow(HWND hWnd) override;
};
