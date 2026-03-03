// WindowsProject1.cpp : Defines the entry point for the application.
//

#include "WindowsProject1.h"
#include "framework.h"

#include "AudioUtils.h"

#include <shlobj.h>
#include <windowsx.h>

#include <algorithm>
#include <iostream>
#include <string>

#define MAX_LOADSTRING 100
#define HEX_TO_RGB(hex) RGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF)

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

HINSTANCE hInst; // current instance
WCHAR szTitle[MAX_LOADSTRING]; // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING]; // the main window class name

// darks  0x0A0E15 0x212631 0x373F4E 0x4E576A 0x667085
// lights 0xBFC6D4 0xD1d6E9 0xE0E4EB 0xF0F1F5 0xFFFFFF
HBRUSH hBrushBackground = CreateSolidBrush(HEX_TO_RGB(0x373F4E));
HBRUSH hBrushCaption = CreateSolidBrush(HEX_TO_RGB(0x4E576A));
int captionSizeLeft = 80;

HCURSOR cursorDefault = LoadCursor(nullptr, IDC_ARROW);
HCURSOR cursorHand = LoadCursor(nullptr, IDC_HAND);
HCURSOR cursorDrag = LoadCursor(nullptr, IDC_SIZEALL);

// Helper to get the full path to our settings file
std::wstring GetIniPath()
{
    PWSTR appDataPath = NULL;
    std::wstring fullPath = L"";
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
        fullPath = std::wstring(appDataPath) + L"\\YourAppName.ini";
        CoTaskMemFree(appDataPath);
    }
    return fullPath;
}

void LoadWindowRect(HWND hwnd)
{
    std::wstring iniPath = GetIniPath();
    if (iniPath.empty())
        return;

    RECT rect;
    rect.left = GetPrivateProfileIntW(L"Window", L"x", 100, iniPath.c_str());
    rect.top = GetPrivateProfileIntW(L"Window", L"y", 100, iniPath.c_str());
    int width = GetPrivateProfileIntW(L"Window", L"w", 800, iniPath.c_str());
    int height = GetPrivateProfileIntW(L"Window", L"h", 600, iniPath.c_str());
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;

    HMONITOR hMonitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONULL);
    if (hMonitor == NULL) {
        rect.left = 100;
        rect.top = 100;

    } else {
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfo(hMonitor, &mi)) {
            if (rect.right > mi.rcWork.right)
                rect.left = mi.rcWork.right - width;
            if (rect.bottom > mi.rcWork.bottom)
                rect.top = mi.rcWork.bottom - height;
            if (rect.left < mi.rcWork.left)
                rect.left = mi.rcWork.left;
            if (rect.top < mi.rcWork.top)
                rect.top = mi.rcWork.top;
        }
    }

    SetWindowPos(hwnd, NULL, rect.left, rect.top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

void SaveWindowRect(HWND hwnd)
{
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        std::wstring iniPath = GetIniPath();
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        WritePrivateProfileStringW(L"Window", L"x", std::to_wstring(rect.left).c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"Window", L"y", std::to_wstring(rect.top).c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"Window", L"w", std::to_wstring(width).c_str(), iniPath.c_str());
        WritePrivateProfileStringW(L"Window", L"h", std::to_wstring(height).c_str(), iniPath.c_str());
    }
}

void CreateConsole()
{
    AllocConsole();
    SetConsoleTitleW(L"Console title");
    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::ios::sync_with_stdio();
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    CreateConsole();

    CoinitializeWrapper coinitializeRAII;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);

    HWND hWnd;
    {
        WNDCLASSEXW wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = hInstance;
        wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
        wcex.hCursor = cursorDefault;
        wcex.hbrBackground = hBrushBackground;
        wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = szWindowClass;
        wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
        RegisterClassExW(&wcex);

        hInst = hInstance;
        // WS_POPUP: Removes the title bar
        // WS_THICKFRAME: Adds the invisible resizing borders
        // WS_SYSMENU: Keeps it on the taskbar/system integration
        // was WS_OVERLAPPEDWINDOW

        DWORD dwStyle = WS_POPUP | WS_THICKFRAME | WS_SYSMENU;
        hWnd = CreateWindowExW(WS_EX_TOPMOST, szWindowClass, szTitle, dwStyle,
            650, 200, 800, 400, nullptr, nullptr, hInstance, nullptr);
        if (!hWnd)
            return FALSE;

        LoadWindowRect(hWnd);
        ShowWindow(hWnd, nCmdShow);
        // UpdateWindow(hWnd);
    }

    // PostMessage(hWnd, WM_PAINT, 0, 0); // to draw square initially!

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

HDC memDC;
HBITMAP memBitmap;

HWND hLabel;

POINT cursorScreenPosCaptured;
LONG cursorOffsetAccumulatorY;

SliderManager sliderManager;
SelectInfo selectedSliderInfo;

RECT getSliderRegion(HWND hWnd)
{
    RECT windowRect;
    GetClientRect(hWnd, &windowRect);
    windowRect.left += captionSizeLeft;
    return windowRect;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE: {
        // hLabel = CreateWindow(L"STATIC", L"Drag to move: X: 0, Y: 0",
        //     WS_VISIBLE | WS_CHILD, 20, 20, 300, 20, hWnd, NULL, NULL, NULL);
        AudioUpdateListener::get().init(hWnd);
        memDC = CreateCompatibleDC(nullptr);
        break;
    }

    case WM_LBUTTONDOWN: {
        RECT rect;
        GetClientRect(hWnd, &rect);
        POINT cursorScreenPos;
        GetCursorPos(&cursorScreenPos);
        POINT cursorClientPos = cursorScreenPos;
        ScreenToClient(hWnd, &cursorClientPos);

        if (auto newCaptured = sliderManager.getHoveredSlider(cursorClientPos)) {
            selectedSliderInfo = newCaptured;
            cursorScreenPosCaptured = cursorScreenPos;
            SetTimer(hWnd, IDT_TIMER_1, 25, (TIMERPROC)NULL);
            SetCapture(hWnd);
            ShowCursor(FALSE);
        }
        break;
    }

    case WM_LBUTTONUP: {
        if (selectedSliderInfo) {
            selectedSliderInfo = {};
            ReleaseCapture();
            ShowCursor(TRUE);
            KillTimer(hWnd, IDT_TIMER_1);
            cursorOffsetAccumulatorY = 0;
        }
        break;
    }

    case WM_MOUSEMOVE: {
        POINT cursorScreenPos;
        GetCursorPos(&cursorScreenPos);
        if (selectedSliderInfo) {
            if (int dy = cursorScreenPos.y - cursorScreenPosCaptured.y) {
                cursorOffsetAccumulatorY -= dy;
                SetCursorPos(cursorScreenPosCaptured.x, cursorScreenPosCaptured.y);
            }
        }
        break;
    }

    case WM_TIMER: {
        if (wParam == IDT_TIMER_1) {
            if (cursorOffsetAccumulatorY) {
                if (auto slider = sliderManager.getGetBySelectInfo(selectedSliderInfo)) {
                    float sliderHeight = slider->getHeight();
                    float newVal = std::clamp(slider->_val + (float)cursorOffsetAccumulatorY / sliderHeight, 0.f, 1.f);
                    if (newVal != slider->_val)
                        AudioUpdateListener::get().setVol(selectedSliderInfo, newVal);
                }
            }
            cursorOffsetAccumulatorY = 0;
        }
    } break;

    case WM_APP_REGISTERED: {
        AudioUpdateInfo info(wParam, lParam);
        sliderManager.addAppSlider(info._pid, info._vol, info._isMuted);
        sliderManager.recalculateSliderRects(getSliderRegion(hWnd));
        InvalidateRect(hWnd, NULL, FALSE);
    } break;

    case WM_APP_UNREGISTERED: {
        AudioUpdateInfo info(wParam, lParam);
        sliderManager.removeAppSlider(info._pid);
        sliderManager.recalculateSliderRects(getSliderRegion(hWnd));
        InvalidateRect(hWnd, NULL, FALSE);
        AudioUpdateListener::get().cleanupExpiredSessions();
    } break;

    case WM_REFRESH_VOL: {
        AudioUpdateInfo info(wParam, lParam);
        SelectInfo si(info._type, info._pid);
        if (auto slider = sliderManager.getGetBySelectInfo(si))
            slider->_val = info._vol;
        InvalidateRect(hWnd, NULL, FALSE); // UpdateWindow(hWnd); // works without it
        return 0;
    }

#define BORDERLESS 1
#if BORDERLESS == 1
    case WM_NCCALCSIZE: {
        if (wParam == TRUE)
            return 0; // Removes the default standard frame padding
        break;
    }
#endif

    case WM_NCHITTEST: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ScreenToClient(hWnd, &pt);

        RECT rc;
        GetClientRect(hWnd, &rc);
#if BORDERLESS == 1
        // clang-format off
        const int border = 8; // resize thickness
        bool left   = pt.x < border;
        bool right  = pt.x >= rc.right - border;
        bool top    = pt.y < border;
        bool bottom = pt.y >= rc.bottom - border;
        if (top && left)     return HTTOPLEFT;
        if (top && right)    return HTTOPRIGHT;
        if (bottom && left)  return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left)            return HTLEFT;
        if (right)           return HTRIGHT;
        if (top)             return HTTOP;
        if (bottom)          return HTBOTTOM;
        // clang-format on
#endif
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam); // handle resize first
        if (hit == HTCLIENT) {
            if (pt.x < captionSizeLeft) {
                return HTCAPTION; // drag one
            }
        }
        return hit;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCAPTION) {
            SetCursor(cursorDrag);
            return TRUE;
        } else if (LOWORD(lParam) == HTCLIENT) {
            POINT cursorClientPos;
            GetCursorPos(&cursorClientPos);
            ScreenToClient(hWnd, &cursorClientPos);
            auto newHoverInfo = sliderManager.getHoveredSlider(cursorClientPos);
            SetCursor(newHoverInfo ? cursorHand : cursorDefault);
            return TRUE;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_DESTROY: {
        AudioUpdateListener::get().uninit();
        IconManager::get().uninit();
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        SaveWindowRect(hWnd);
        PostQuitMessage(0);
    } break;

    case WM_SIZE: {
        if (memBitmap)
            DeleteObject(memBitmap);

        HDC hdc = GetDC(hWnd);
        memBitmap = CreateCompatibleBitmap(hdc, LOWORD(lParam), HIWORD(lParam));
        ReleaseDC(hWnd, hdc);

        sliderManager.recalculateSliderRects(getSliderRegion(hWnd));
    } break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT windowRect;
        GetClientRect(hWnd, &windowRect);
        RECT captionRect { windowRect.left, windowRect.top, windowRect.left + captionSizeLeft, windowRect.bottom };

        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
        FillRect(memDC, &windowRect, hBrushBackground);
        FillRect(memDC, &captionRect, hBrushCaption);

        sliderManager.drawSliders(memDC);
        BitBlt(hdc, 0, 0, windowRect.right, windowRect.bottom, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBitmap);

        EndPaint(hWnd, &ps);
    } break;

    case WM_ERASEBKGND: {
        return 1; // no redraw bkg
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;

        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    } break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
