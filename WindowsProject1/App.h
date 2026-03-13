#pragma once

#include <Windows.h>
#include <windowsx.h>

#include "AudioUtils.h"
#include "Utils.h"

#include <algorithm>

#define HEX_TO_RGB(hex) RGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF)

// darks  0x0A0E15 0x212631 0x373F4E 0x4E576A 0x667085
// lights 0xBFC6D4 0xD1d6E9 0xE0E4EB 0xF0F1F5 0xFFFFFF

class App {
    // temporary
    HBRUSH hBrushBackground = CreateSolidBrush(HEX_TO_RGB(0x373F4E));
    HBRUSH hBrushCaption = CreateSolidBrush(HEX_TO_RGB(0x4E576A));

protected:
    HINSTANCE _hInstance;
    HWND _hWnd;

    AudioUpdateListener _audioAppListerner;

    SliderManager sliderManager;
    SelectInfo sliderInfoHovered;

private:
    bool _mouseTracking;

protected:
    virtual void onMouseLeave();
    virtual void onMouseMove(POINT cursorClientPos, bool justEntered);
    virtual void onMouseScroll(POINT cursorClientPos, float delta);

    virtual void onPaint(HDC hdc)
    {
        RECT windowRect;
        GetClientRect(_hWnd, &windowRect);

        FillRect(hdc, &windowRect, hBrushBackground);
        sliderManager.drawSliders(hdc);
    }
    virtual void onResize(RECT newRect) { }

public:
    HINSTANCE getInstance() { return _hInstance; }

    virtual void initWindow(HINSTANCE instance, WNDPROC wndProc, int nCmdShow);

    virtual void handleResize(WPARAM wParam, LPARAM lParam);
    //virtual void handleClose(HWND hWnd);
    virtual void handleDestroy(HWND hWnd);

    void handleMouseMove(WPARAM wParam, LPARAM lParam);
    void handleMouseLeave();
    void handleMouseScroll(WPARAM wParam, LPARAM lParam);

    void handlePaint();

    void setWindowAlpha(BYTE alpha);

public: // INHERIT THIS
    void handleMMAppRegistered(WPARAM wParam, LPARAM lParam);
    void handleMMAppUnegistered(WPARAM wParam, LPARAM lParam);
    void handleMMRefreshVol(WPARAM wParam, LPARAM lParam);
};