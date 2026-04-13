// TinyVolumeApp.cpp : Defines the entry point for the application.

#include "Resource.h"

#include "VolumeApp.h"

#if defined(_DEBUG)
#define LOG_ENABLED 1
#else
#define LOG_ENABLED 0
#endif
#include "Timer.h"

VolumeApp app;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

#include <iostream>
void createConsole()
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
#if defined(_DEBUG)
    createConsole();
#endif

    static CoinitializeWrapper coinitializeRAII;

    app.construct(hInstance, WndProc);

    TIMEPOINT("init window");
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TINY_VOLUME_APP));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE: {
        TIMEPOINT("WM_CREATE");
    } break;

    case WM_CLOSE: {
        app.destroyWindow(hWnd);
    } break;

    case WM_LBUTTONDOWN: {
        app.handleMouseButton(wParam, lParam, MouseButton::Left, true);
        return 0;
    } break;

    case WM_LBUTTONUP: {
        app.handleMouseButton(wParam, lParam, MouseButton::Left, false);
        return 0;
    } break;

    case WM_RBUTTONDOWN: {
        app.handleMouseButton(wParam, lParam, MouseButton::Right, true);
        return 0;
    } break;

    case WM_RBUTTONUP: {
        app.handleMouseButton(wParam, lParam, MouseButton::Right, false);
        return 0;
    } break;

    case WM_MBUTTONDOWN: {
        app.handleMouseButton(wParam, lParam, MouseButton::Mid, true);
        return 0;
    } break;

    case WM_MBUTTONUP: {
        app.handleMouseButton(wParam, lParam, MouseButton::Mid, false);
        return 0;
    } break;

    case WM_MOUSEMOVE: {
        static LPARAM prevLParam;
        if (prevLParam != lParam) {
            app.handleMouseMove(wParam, lParam);
            prevLParam = lParam;
        }
        return 0;
    } break;

    case WM_MOUSELEAVE: {
        app.handleMouseLeave();
    } break;

    case WM_MOUSEWHEEL: {
        app.handleMouseScroll(wParam, lParam);
    } break;

    case WM_APP_REGISTERED: {
        app.handleMMAppRegistered((AudioSessionInitInfo*)wParam);
    } break;

    case WM_APP_UNREGISTERED: {
        app.handleMMAppUnegistered(wParam, lParam);
    } break;

    case WM_APP_ACTIVATION_CHANGED: {
        app.handleMMAppActivationChanged(wParam, lParam);
    } break;

    case WM_REFRESH_VOL: {
        app.handleMMRefreshVol(wParam, lParam);
    } break;

    case WM_DESTROY: {
        // app.handleDestroy(hWnd);
        PostQuitMessage(0);
    } break;

    case WM_SIZE: {
        app.handleResize(wParam, lParam);
    } break;

    case WM_PAINT: {
        TIMEPOINT("WM_PAINT");
        app.handlePaint();
    } break;

    case WM_ERASEBKGND: {
        return 1; // no redraw bkg
    } break;

    case WM_NCCALCSIZE: {
        return 0;
    } break;

    case WM_NCHITTEST: {
        return app.handleNCAHitTest(hWnd, lParam);
    } break;

    case WM_GETMINMAXINFO: {
        app.handleMinMaxInfo(wParam, lParam);
        return 0;
    } break;

    case WM_TIMER: {
        if (wParam == WM_TIMER_UI)
            app.handleTimerUpdateUI();
    } break;

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(app.getInstance(), MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;

        case IDM_EXIT:
            SendMessage(hWnd, WM_CLOSE, wParam, lParam);
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
