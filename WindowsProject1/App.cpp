#include "App.h"

#include "Resource.h"
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
    LoadStringW(_hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);

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

void App::handleLMB(WPARAM wParam, LPARAM lParam, bool down)
{
    ReleaseCapture();
    SendMessage(_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0); // handle window drag on LMB
}

void App::handleRMB(WPARAM wParam, LPARAM lParam, bool down)
{
    ReleaseCapture();
    SendMessage(_hWnd, WM_CLOSE, 0, 0);
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

void App::handlePaint()
{
    PAINTSTRUCT ps;
    HDC hdcScreen = BeginPaint(_hWnd, &ps);

    RECT wndRect;
    GetWindowRect(_hWnd, &wndRect);
    auto width = wndRect.right - wndRect.left;
    auto height = wndRect.bottom - wndRect.top;

    // HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // Create a 32-bit DIB (ARGB)
    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(bih);
    bih.biWidth = width;
    bih.biHeight = -height; // top-down
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;

    BITMAPINFO bi = {};
    bi.bmiHeader = bih;

    DWORD* pixels = nullptr;
    HBITMAP hbm = CreateDIBSection(hdcScreen, &bi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
    HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hbm);

    Canvas canvas { (DWORD*)pixels, width, height };

    if (pixels)
        onPaint(hdcMem, canvas);

    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT ptDst = { wndRect.left, wndRect.top };
    POINT ptSrc = { 0, 0 };
    SIZE szWnd = { width, height };

    UpdateLayeredWindow(_hWnd, hdcScreen, &ptDst, &szWnd,
        hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    // Cleanup
    SelectObject(hdcMem, hOld);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    EndPaint(_hWnd, &ps);
}

// INHERIT BELOW

void App::handleResize(WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(_hWnd, &rc);
    onResize(rc);
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
