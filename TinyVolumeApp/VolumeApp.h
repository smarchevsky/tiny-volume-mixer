#pragma once

#include "App.h"

#include "AudioUtils.h"
#include "HitDetector.h"
#include "SliderManager.h"
#include "UIManager.h"

class Button {
    BufferRLE _rleSolid;
    BufferRLE _rleBordered;
    DWORD _colorDefault, _colorSemiTransparent;
    SIZE _imageSize;
    POINT _pos;

public:
    HitUID _hitUID;
    enum State : uint8_t {
        Default,
        Hovered,
        Pressed
    } _currentState;

    void setPos(POINT pos) { _pos = pos; }
    void draw(HDC hdc);
    void initialize(std::vector<DWORD>& pixels, int resourceID, SIZE size, int cr, int bw);
};

class VolumeApp : public App {
    AudioUpdateListener _audioAppListerner;
    UIConfig _uic;
    SliderManager _sliderManager;

    Button _btnClose {}, _btnSettings {};

    HitDetector _hitDetector;
    HitUID _hitHovered = HitUID_none;
    bool _isAppHovered;

    virtual void onPaint(HDC hdc) override;
    virtual void onResize(RECT newRect) override;

    virtual void onMouseLeave() override;
    virtual void onMouseMove(POINT cursorClientPos, bool justEntered) override;
    virtual void onMouseScroll(POINT cursorClientPos, float delta) override;

    void recalculateHitRects(const RECT& rc);

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
