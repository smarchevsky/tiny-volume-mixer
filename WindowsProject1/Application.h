#pragma once

#include <Windows.h>
#include <windowsx.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE) & __ImageBase)

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME (0x00AF)
#endif

class Application {
public:
    HWND _hWnd;
    int _width, _height;
    RECT _rgn;
    bool _themeEnabled, _compositionEnabled;

    int init(WNDPROC proc);

    void update_region();

    void handle_nccreate(HWND _hWnd, CREATESTRUCTW* cs) { SetWindowLongPtrW(_hWnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams); }

    void handle_compositionchanged();

    bool handle_keydown(DWORD key);

    bool has_autohide_appbar(UINT edge, RECT mon);

    void handle_nccalcsize(WPARAM wparam, LPARAM lparam);

    LRESULT handle_nchittest(int x, int y);

    void handle_paint();

    void handle_themechanged();

    void handle_windowposchanged(const WINDOWPOS* pos);

    LRESULT handle_message_invisible(HWND hWnd, UINT msg, WPARAM wparam,
        LPARAM lparam);
};
