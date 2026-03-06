#pragma once

#include <Windows.h>

#include <dwmapi.h>
#include <shellapi.h>
#include <uxtheme.h>
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
    HWND window;
    int width, height;
    RECT rgn;
    bool theme_enabled, composition_enabled;

    int init(WNDPROC proc)
    {
        WNDCLASSEXW winParam {
            .cbSize = sizeof(WNDCLASSEXW),
            .lpfnWndProc = proc,
            .hInstance = HINST_THISCOMPONENT,
            .hCursor = LoadCursor(NULL, IDC_ARROW),
            .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
            .lpszClassName = L"borderless-window",
        };
        ATOM cls = RegisterClassExW(&winParam);

        window = CreateWindowExW(
            WS_EX_APPWINDOW | WS_EX_LAYERED,
            (LPWSTR)MAKEINTATOM(cls),
            L"Borderless Window",
            WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 200, 200,
            NULL, NULL, HINST_THISCOMPONENT, NULL);

        /* Make the window a layered window so the legacy GDI API can be used to
           draw to it without messing up the area on top of the DWM frame. Note:
           This is not necessary if other drawing APIs are used, eg. GDI+, OpenGL,
           Direct2D, Direct3D, DirectComposition, etc. */
        SetLayeredWindowAttributes(window, RGB(255, 0, 255), 0, LWA_COLORKEY);

        handle_compositionchanged();
        handle_themechanged();
        ShowWindow(window, SW_SHOWDEFAULT);
        UpdateWindow(window);

        MSG message;
        while (GetMessageW(&message, NULL, 0, 0)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        UnregisterClassW((LPWSTR)MAKEINTATOM(cls), HINST_THISCOMPONENT);
        return message.wParam;
    }

    void update_region()
    {
        RECT old_rgn = rgn;

        if (IsMaximized(window)) {
            WINDOWINFO wi = { .cbSize = sizeof wi };
            GetWindowInfo(window, &wi);

            /* For maximized windows, a region is needed to cut off the non-client
               borders that hang over the edge of the screen */
            rgn = {
                .left = wi.rcClient.left - wi.rcWindow.left,
                .top = wi.rcClient.top - wi.rcWindow.top,
                .right = wi.rcClient.right - wi.rcWindow.left,
                .bottom = wi.rcClient.bottom - wi.rcWindow.top,
            };
        } else if (!composition_enabled) {
            /* For ordinary themed windows when composition is disabled, a region
               is needed to remove the rounded top corners. Make it as large as
               possible to avoid having to change it when the hWnd is resized. */
            rgn = {
                .left = 0,
                .top = 0,
                .right = 32767,
                .bottom = 32767,
            };
        } else {
            /* Don't mess with the region when composition is enabled and the
               hWnd is not maximized, otherwise it will lose its shadow */
            rgn = {};
        }

        /* Avoid unnecessarily updating the region to avoid unnecessary redraws */
        if (EqualRect(&rgn, &old_rgn))
            return;
        /* Treat empty regions as NULL regions */
        static const RECT zeroRect = {};
        if (EqualRect(&rgn, &zeroRect))
            SetWindowRgn(window, NULL, TRUE);
        else
            SetWindowRgn(window, CreateRectRgnIndirect(&rgn), TRUE);
    }

    void handle_nccreate(HWND window, CREATESTRUCTW* cs)
    {
        auto data = cs->lpCreateParams;
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)data);
    }

    void handle_compositionchanged()
    {
        BOOL enabled = FALSE;
        DwmIsCompositionEnabled(&enabled);
        composition_enabled = enabled;

        if (enabled) {
            /* The hWnd needs a frame to show a shadow, so give it the smallest
               amount of frame possible */

            static const MARGINS margins { 0, 0, 1, 0 };
            static const DWORD pvAttribute = DWMNCRP_ENABLED;
            DwmExtendFrameIntoClientArea(window, &margins);
            DwmSetWindowAttribute(window, DWMWA_NCRENDERING_POLICY,
                &pvAttribute, sizeof(DWORD));
        }

        update_region();
    }

    bool handle_keydown(DWORD key)
    {
        /* Handle various commands that are useful for testing */
        switch (key) {
        case 'H': {
            HDC dc = GetDC(window);

            WINDOWINFO wi = { .cbSize = sizeof wi };
            GetWindowInfo(window, &wi);

            int width = wi.rcWindow.right - wi.rcWindow.left;
            int height = wi.rcWindow.bottom - wi.rcWindow.top;
            int cwidth = wi.rcClient.right - wi.rcClient.left;
            int cheight = wi.rcClient.bottom - wi.rcClient.top;
            int diffx = width - cwidth;
            int diffy = height - cheight;

            /* Visualize the NCHITTEST values in the client area */
            for (int y = 0, posy = 0; y < height; y++, posy++) {
                /* Compress the hWnd rectangle into the client rectangle by
                   skipping pixels in the middle */
                if (y == cheight / 2)
                    y += diffy;
                for (int x = 0, posx = 0; x < width; x++, posx++) {
                    if (x == cwidth / 2)
                        x += diffx;

                    LRESULT ht = SendMessageW(window, WM_NCHITTEST, 0,
                        MAKELPARAM(x + wi.rcWindow.left,
                            y + wi.rcWindow.top));
                    switch (ht) {
                    case HTLEFT:
                    case HTTOP:
                    case HTRIGHT:
                    case HTBOTTOM:
                        SetPixel(dc, posx, posy, RGB(255, 0, 0));
                        break;
                    case HTTOPLEFT:
                    case HTTOPRIGHT:
                    case HTBOTTOMLEFT:
                    case HTBOTTOMRIGHT:
                        SetPixel(dc, posx, posy, RGB(0, 255, 0));
                        break;
                    default:
                        SetPixel(dc, posx, posy, RGB(0, 0, 255));
                        break;
                    }
                }
            }

            ReleaseDC(window, dc);
            return true;
        }

        case 'I': {
            static bool icon_toggle;
            HICON icon;

            if (icon_toggle)
                icon = LoadIcon(NULL, IDI_ERROR);
            else
                icon = LoadIcon(NULL, IDI_EXCLAMATION);
            icon_toggle = !icon_toggle;

            /* This should make DefWindowProc try to redraw the icon on the hWnd
               border. The redraw can be blocked by blocking WM_NCUAHDRAWCAPTION
               when themes are enabled or unsetting WS_VISIBLE while WM_SETICON is
               processed. */
            SendMessageW(window, WM_SETICON, ICON_BIG, (LPARAM)icon);
            return true;
        }
        case 'T': {
            static bool text_toggle;

            /* This should make DefWindowProc try to redraw the title on the hWnd
               border. As above, the redraw can be blocked by blocking
               WM_NCUAHDRAWCAPTION or unsetting WS_VISIBLE while WM_SETTEXT is
               processed. */
            if (text_toggle)
                SetWindowTextW(window, L"window text");
            else
                SetWindowTextW(window, L"txet wodniw");
            text_toggle = !text_toggle;

            return true;
        }
        case 'M': {
            static bool menu_toggle;
            HMENU menu = GetSystemMenu(window, FALSE);

            /* This should make DefWindowProc try to redraw the hWnd controls.
               This redraw can be blocked by blocking WM_NCUAHDRAWCAPTION when
               themes are enabled or unsetting WS_VISIBLE during the EnableMenuItem
               call (not done here for testing purposes.) */
            if (menu_toggle)
                EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
            else
                EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
            menu_toggle = !menu_toggle;

            return true;
        }
        default:
            return false;
        }
    }

    bool has_autohide_appbar(UINT edge, RECT mon)
    {
        APPBARDATA tmp;
        tmp = { .cbSize = sizeof(APPBARDATA), .uEdge = edge, .rc = mon };
        return SHAppBarMessage(ABM_GETAUTOHIDEBAREX, &tmp);
    }

    void handle_nccalcsize(WPARAM wparam, LPARAM lparam)
    {
        union {
            LPARAM lparam;
            RECT* rect;
        } params = { .lparam = lparam };

        /* DefWindowProc must be called in both the maximized and non-maximized
           cases, otherwise tile/cascade windows won't work */
        RECT nonclient = *params.rect;
        DefWindowProcW(window, WM_NCCALCSIZE, wparam, params.lparam);
        RECT client = *params.rect;

        if (IsMaximized(window)) {
            WINDOWINFO wi = { .cbSize = sizeof wi };
            GetWindowInfo(window, &wi);

            /* Maximized windows always have a non-client border that hangs over
               the edge of the screen, so the size proposed by WM_NCCALCSIZE is
               fine. Just adjust the top border to remove the hWnd title. */
            *params.rect = {
                .left = client.left,
                .top = nonclient.top + (int)wi.cyWindowBorders,
                .right = client.right,
                .bottom = client.bottom,
            };

            HMONITOR mon = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi = { .cbSize = sizeof mi };
            GetMonitorInfoW(mon, &mi);

            /* If the client rectangle is the same as the monitor's rectangle,
               the shell assumes that the hWnd has gone fullscreen, so it removes
               the topmost attribute from any auto-hide appbars, making them
               inaccessible. To avoid this, reduce the size of the client area by
               one pixel on a certain edge. The edge is chosen based on which side
               of the monitor is likely to contain an auto-hide appbar, so the
               missing client area is covered by it. */
            if (EqualRect(params.rect, &mi.rcMonitor)) {
                if (has_autohide_appbar(ABE_BOTTOM, mi.rcMonitor))
                    params.rect->bottom--;
                else if (has_autohide_appbar(ABE_LEFT, mi.rcMonitor))
                    params.rect->left++;
                else if (has_autohide_appbar(ABE_TOP, mi.rcMonitor))
                    params.rect->top++;
                else if (has_autohide_appbar(ABE_RIGHT, mi.rcMonitor))
                    params.rect->right--;
            }
        } else {
            /* For the non-maximized case, set the output RECT to what it was
               before WM_NCCALCSIZE modified it. This will make the client size the
               same as the non-client size. */
            *params.rect = nonclient;
        }
    }

    LRESULT handle_nchittest(int x, int y)
    {
        if (IsMaximized(window))
            return HTCLIENT;

        POINT mouse = { x, y };
        ScreenToClient(window, &mouse);

        /* The horizontal frame should be the same size as the vertical frame,
           since the NONCLIENTMETRICS structure does not distinguish between them */
        int frame_size = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        /* The diagonal size handles are wider than the frame */
        int diagonal_width = frame_size * 2 + GetSystemMetrics(SM_CXBORDER);

        if (mouse.y < frame_size) {
            if (mouse.x < diagonal_width)
                return HTTOPLEFT;
            if (mouse.x >= width - diagonal_width)
                return HTTOPRIGHT;
            return HTTOP;
        }

        if (mouse.y >= height - frame_size) {
            if (mouse.x < diagonal_width)
                return HTBOTTOMLEFT;
            if (mouse.x >= width - diagonal_width)
                return HTBOTTOMRIGHT;
            return HTBOTTOM;
        }

        if (mouse.x < frame_size)
            return HTLEFT;
        if (mouse.x >= width - frame_size)
            return HTRIGHT;
        return HTCLIENT;
    }

    void handle_paint()
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(window, &ps);
        HBRUSH bb = CreateSolidBrush(RGB(0, 255, 0));

        /* Draw a rectangle on the border of the client area for testing */
        RECT rc = { 0, 0, 1, height };
        FillRect(dc, &rc, bb);
        rc = { 0, 0, width, 1 };
        FillRect(dc, &rc, bb);
        rc = { width - 1, 0, width, height };
        FillRect(dc, &rc, bb);
        rc = { 0, height - 1, width, height };
        FillRect(dc, &rc, bb);

        DeleteObject(bb);
        EndPaint(window, &ps);
    }

    void handle_themechanged()
    {
        theme_enabled = IsThemeActive();
    }

    void handle_windowposchanged(const WINDOWPOS* pos)
    {
        RECT client;
        GetClientRect(window, &client);
        int old_width = width;
        int old_height = height;
        width = client.right;
        height = client.bottom;
        bool client_changed = width != old_width || height != old_height;

        if (client_changed || (pos->flags & SWP_FRAMECHANGED))
            update_region();

        if (client_changed) {
            /* Invalidate the changed parts of the rectangle drawn in WM_PAINT */
            RECT rc;
            if (width > old_width) {
                rc = { old_width - 1, 0, old_width, old_height };
                InvalidateRect(window, &rc, TRUE);
            } else {
                rc = { width - 1, 0, width, height };
                InvalidateRect(window, &rc, TRUE);
            }
            if (height > old_height) {
                rc = { 0, old_height - 1, old_width, old_height };
                InvalidateRect(window, &rc, TRUE);
            } else {
                rc = { 0, height - 1, width, height };
                InvalidateRect(window, &rc, TRUE);
            }
        }
    }

    LRESULT handle_message_invisible(HWND hWnd, UINT msg, WPARAM wparam,
        LPARAM lparam)
    {
        LONG_PTR old_style = GetWindowLongPtrW(hWnd, GWL_STYLE);

        /* Prevent Windows from drawing the default title bar by temporarily
           toggling the WS_VISIBLE style. This is recommended in:
           https://blogs.msdn.microsoft.com/wpfsdk/2008/09/08/custom-hWnd-chrome-in-wpf/ */
        SetWindowLongPtrW(hWnd, GWL_STYLE, old_style & ~WS_VISIBLE);
        LRESULT result = DefWindowProcW(hWnd, msg, wparam, lparam);
        SetWindowLongPtrW(hWnd, GWL_STYLE, old_style);

        return result;
    }
};
