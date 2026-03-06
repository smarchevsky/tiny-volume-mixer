// WindowsProject1.cpp : Defines the entry point for the application.
//

#include "WindowsProject1.h"
#include "framework.h"

#include "Application.h"

#include <algorithm>
#include <iostream>
#include <string>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#define MAX_LOADSTRING 100
#define HEX_TO_RGB(hex) RGB((hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF)

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

Application app;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    return (int)app.init(WndProc);
}

LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_DWMCOMPOSITIONCHANGED:
        app.handle_compositionchanged();
        return 0;
    case WM_KEYDOWN:
        if (app.handle_keydown(wParam))
            return 0;
        break;
    case WM_LBUTTONDOWN:
        /* Allow hWnd dragging from any point */
        ReleaseCapture();
        SendMessageW(window, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        return 0;
    case WM_NCACTIVATE:
        /* DefWindowProc won't repaint the hWnd border if lParam (normally a
           HRGN) is -1. This is recommended in:
           https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-hWnd-chrome-in-wpf/ */
        return DefWindowProcW(window, msg, wParam, -1);
    case WM_NCCALCSIZE:
        app.handle_nccalcsize(wParam, lParam);
        return 0;
    case WM_NCHITTEST:
        return app.handle_nchittest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    case WM_NCPAINT:
        /* Only block WM_NCPAINT when composition is disabled. If it's blocked
           when composition is enabled, the hWnd shadow won't be drawn. */
        if (!app.composition_enabled)
            return 0;
        break;
    case WM_NCUAHDRAWCAPTION:
    case WM_NCUAHDRAWFRAME:
        /* These undocumented messages are sent to draw themed hWnd borders.
           Block them to prevent drawing borders over the client area. */
        return 0;
    case WM_PAINT:
        app.handle_paint();
        return 0;
    case WM_SETICON:
    case WM_SETTEXT:
        /* Disable painting while these messages are handled to prevent them
           from drawing a hWnd caption over the client area, but only when
           composition and theming are disabled. These messages don't paint
           when composition is enabled and blocking WM_NCUAHDRAWCAPTION should
           be enough to prevent painting when theming is enabled. */
        if (!app.composition_enabled && !app.theme_enabled)
            return app.handle_message_invisible(window, msg, wParam, lParam);
        break;
    case WM_THEMECHANGED:
        app.handle_themechanged();
        break;
    case WM_WINDOWPOSCHANGED:
        app.handle_windowposchanged((WINDOWPOS*)lParam);
        return 0;
    }

    return DefWindowProcW(window, msg, wParam, lParam);
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
