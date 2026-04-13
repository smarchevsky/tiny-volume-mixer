#include "App.h"

#include "Resource.h"
#include "Utils.h"
// #include <cassert>

HCURSOR cursorDefault = LoadCursor(nullptr, IDC_ARROW);
HCURSOR cursorHand = LoadCursor(nullptr, IDC_HAND);

#define MAX_LOADSTRING 100
WCHAR szTitle[MAX_LOADSTRING]; // title bar text
WCHAR szWindowClass[MAX_LOADSTRING]; // main window class name

void App::initWindow(HINSTANCE instance, WNDPROC wndProc, RECT rc)
{
    _hInstance = instance;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    LoadStringW(_hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(_hInstance, IDC_TINY_VOLUME_APP, szWindowClass, MAX_LOADSTRING);

    auto icon = LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON_MULTIRES));
    WNDCLASSEXW wcex {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = wndProc;
    wcex.hInstance = _hInstance;
    wcex.lpszClassName = szWindowClass;
    wcex.hCursor = cursorDefault;
    wcex.hIcon = icon;
    wcex.hIconSm = icon;
    RegisterClassExW(&wcex);

    _hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        szWindowClass, szTitle,
        WS_POPUP | WS_VISIBLE,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, _hInstance, nullptr);

    _clipRegion = CreateRectRgn(0, 0, 0, 0);
}

void App::destroyWindow(HWND hWnd)
{
    if (_hWnd == hWnd) {
        DestroyWindow(_hWnd);
        _hWnd = nullptr;
    }
}

void App::handleMouseLeave()
{
    _mouseTracking = false;
    onMouseLeave();
}

void App::handleMouseScroll(WPARAM wParam, LPARAM lParam)
{
    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    float wheelSteps = (float)zDelta / WHEEL_DELTA;
    POINT cursorScreenPos { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    ScreenToClient(_hWnd, &cursorScreenPos);
    onMouseScroll(cursorScreenPos, wheelSteps);
}

void App::handleMouseMove(WPARAM wParam, LPARAM lParam)
{
    bool justEntered = false;
    if (!_mouseTracking) {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = _hWnd;
        if (TrackMouseEvent(&tme)) {
            _mouseTracking = true;
            justEntered = true;
        }
    }

    POINT cursorScreenPos { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    // printf("cursorScreenPos %s %d, %d\n", (justEntered ? "entered" : ""), cursorScreenPos.x, cursorScreenPos.y);
    onMouseMove(cursorScreenPos, justEntered);
}

static void debugUpdateRegion(LONG w, LONG h, DWORD* pixels)
{
    int seed = rand();
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            int hash = (x / 4) * (31 + seed % 4) + (y / 4);
            if (hash % 12 == 0) {
                DWORD& pixel = pixels[y * w + x];
                pixel = (pixel & 0xFF000000) | (0xFFFFFF & ((seed << 16) + seed));
            }
        }
}

void App::handlePaint()
{
    RECT r;
    GetUpdateRect(_hWnd, &r, FALSE);

    PAINTSTRUCT ps;
    HDC hdcScreen = BeginPaint(_hWnd, &ps);

    RECT wndRect;
    GetWindowRect(_hWnd, &wndRect);
    auto width = wndRect.right - wndRect.left;
    auto height = wndRect.bottom - wndRect.top;

    HBITMAP hOld = (HBITMAP)SelectObject(_hdcMem, _hbm);

    DIBSECTION ds;
    if (GetObject(_hbm, sizeof(DIBSECTION), &ds)) {
        DWORD* pixels = (DWORD*)ds.dsBm.bmBits;

        SetRectRgn(_clipRegion, r.left, r.top, r.right, r.bottom);
        SelectClipRgn(_hdcMem, _clipRegion);

        memset(pixels, 0, width * height * sizeof(*pixels));
        onPaint(_hdcMem);

#if (_DEBUG)
        debugUpdateRegion(width, height, pixels);
#endif

        BLENDFUNCTION blend = {};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        POINT ptDst = { wndRect.left, wndRect.top };
        POINT ptSrc = { 0, 0 };
        SIZE szWnd = { width, height };

        UPDATELAYEREDWINDOWINFO info = {};
        info.cbSize = sizeof(info);
        info.hdcSrc = _hdcMem;
        info.pptSrc = &ptSrc;
        info.hdcDst = hdcScreen;
        info.pptDst = &ptDst;
        info.psize = &szWnd;
        info.pblend = &blend;
        info.dwFlags = ULW_ALPHA;
        info.prcDirty = &r;

        UpdateLayeredWindowIndirect(_hWnd, &info);
        // UpdateLayeredWindow(_hWnd, hdcScreen, &ptDst, &szWnd, _hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);
    }
    SelectObject(_hdcMem, hOld);

    EndPaint(_hWnd, &ps);
}

// INHERIT BELOW

void App::handleResize(WPARAM wParam, LPARAM lParam)
{
    SIZE size { LOWORD(lParam), HIWORD(lParam) };

    DeleteObject(_hbm);
    DeleteDC(_hdcMem);

    _hdcMem = CreateCompatibleDC(NULL);
    BITMAPINFO bi = getBMI_ARGB(size);
    _hbm = CreateDIBSection(_hdcMem, &bi, DIB_RGB_COLORS, (void**)0, NULL, 0);

    onResize({ 0, 0, size.cx, size.cy });
}

LRESULT App::handleNCAHitTest(HWND hWnd, LPARAM lParam)
{
    if (hWnd == _hWnd) {
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(_hWnd, &pt);
        RECT rc;
        GetWindowRect(_hWnd, &rc);

        const int _width = rc.right - rc.left, _height = rc.bottom - rc.top;
        const int frame_size = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        const int diagonal_width = frame_size * 2 + GetSystemMetrics(SM_CXBORDER);

        if (pt.y < frame_size) {
            if (pt.x < diagonal_width)
                return HTTOPLEFT;
            if (pt.x >= _width - diagonal_width)
                return HTTOPRIGHT;
            return HTTOP;
        }

        if (pt.y >= _height - frame_size) {
            if (pt.x < diagonal_width)
                return HTBOTTOMLEFT;
            if (pt.x >= _width - diagonal_width)
                return HTBOTTOMRIGHT;
            return HTBOTTOM;
        }

        if (pt.x < frame_size)
            return HTLEFT;
        if (pt.x >= _width - frame_size)
            return HTRIGHT;
        return HTCLIENT;
    }

    return HTCAPTION;
}

void App::handleMinMaxInfo(WPARAM wParam, LPARAM lParam)
{
    MINMAXINFO* mmi = (MINMAXINFO*)lParam;
    mmi->ptMinTrackSize.x = 300;
    mmi->ptMinTrackSize.y = 200;
    mmi->ptMaxTrackSize.x = 1280;
    mmi->ptMaxTrackSize.y = 600;
}
