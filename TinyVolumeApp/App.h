#pragma once

#include <Windows.h>
#include <windowsx.h>

#include "Draw.h"

#include <algorithm>

#define HEX_TO_RGB(hex) RGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF)

// darks  0x0A0E15 0x212631 0x373F4E 0x4E576A 0x667085
// lights 0xBFC6D4 0xD1d6E9 0xE0E4EB 0xF0F1F5 0xFFFFFF

enum class MouseButton : uint8_t {
    Left = 1,
    Right = 2,
    Mid = 4
};
class App {
protected:
    HINSTANCE _hInstance;
    HWND _hWnd;

    HBITMAP _hbm;
    HDC _hdcMem;
    HRGN _clipRegion;

private:
    bool _mouseTracking;

protected:
    virtual void onMouseLeave() = 0;
    virtual void onMouseMove(POINT cursorClientPos, bool justEntered) = 0;
    virtual void onMouseScroll(POINT cursorClientPos, float delta) = 0;
    virtual void onMouseButton(POINT cursorClientPos, MouseButton btn, bool down) = 0;

    virtual void onPaint(HDC hdc) = 0;
    virtual void onResize(RECT newRect) = 0;

public:
    virtual void initWindow(HINSTANCE instance, WNDPROC wndProc, RECT rc);
    virtual void destroyWindow(HWND hWnd);

    HINSTANCE getInstance() { return _hInstance; }

    virtual void handleResize(WPARAM wParam, LPARAM lParam);

    void handleMouseMove(WPARAM wParam, LPARAM lParam);
    void handleMouseLeave();
    void handleMouseScroll(WPARAM wParam, LPARAM lParam);
    void handleMouseButton(WPARAM wParam, LPARAM lParam, MouseButton btn, bool down) { onMouseButton(POINT { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, btn, down); }

    LRESULT handleNCAHitTest(HWND hWnd, LPARAM lParam);
    void handleMinMaxInfo(WPARAM wParam, LPARAM lParam);

    void handlePaint();
};