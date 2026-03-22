#include "VolumeApp.h"

HBRUSH hBrushBackground = CreateSolidBrush(HEX_TO_RGB(0x373F4E));

void VolumeApp::construct(HINSTANCE instance, WNDPROC wndProc)
{
    RECT rc;
    FileManager::get().loadWindowRect(rc);

    initWindow(instance, wndProc, rc);
    // setWindowSemiTransparent(true);

    _audioAppListerner.init(_hWnd);
}

void VolumeApp::destroyWindow(HWND hWnd)
{
    if (_hWnd == hWnd) {
        _audioAppListerner.uninit();

        RECT rc;
        if (GetWindowRect(_hWnd, &rc)) {
            FileManager::get().saveWindowRect(rc);
        }

        DestroyWindow(_hWnd);
        _hWnd = nullptr;
    }
}

void VolumeApp::handleMMAppRegistered(AudioSessionInitInfo* sessionInitInfo)
{
    AudioUpdateInfo& info = sessionInitInfo->updateInfo;

    std::wstring pathStr;

    if (sessionInitInfo->iconPath && wcslen(sessionInitInfo->iconPath)) {
        pathStr = sessionInitInfo->iconPath;

    } else {
        if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, info._pid)) {
            wchar_t exePath[MAX_PATH];
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProcess, 0, exePath, &size))
                pathStr = exePath;
            CloseHandle(hProcess);
        }
    }
    IconInfo iconInfo = IconManager::get().getIconFromPath(std::move(pathStr));
    if (!iconInfo.hLarge)
        iconInfo = IconManager::get().getIconFromPackageInstallPath(info._pid, pathStr);

    // wprintf(L"PATH: %s\n", pathStr.c_str());

    sliderManager.appSliderAdd(info._pid, info._vol, info._isMuted, iconInfo);
    delete sessionInitInfo;

    RECT rc;
    GetClientRect(_hWnd, &rc);
    sliderManager.recalculateSliderRects(rc);

    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::handleMMAppUnegistered(WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(_hWnd, &rc);

    AudioUpdateInfo info(wParam, lParam);
    sliderManager.appSliderRemove(info._pid);
    sliderManager.recalculateSliderRects(rc);

    InvalidateRect(_hWnd, NULL, FALSE);
    _audioAppListerner.cleanupExpiredSessions();
}

void VolumeApp::handleMMRefreshVol(WPARAM wParam, LPARAM lParam)
{
    AudioUpdateInfo info(wParam, lParam);
    SelectInfo si(info._type, info._pid);
    if (auto slider = sliderManager.getSliderFromSelect(si)) {
        slider->_val = info._vol;
        InvalidateRect(_hWnd, &slider->_rect, FALSE);
    }
}

void VolumeApp::onPaint(HDC hdc, Canvas canvas)
{
    RECT windowRect { 0, 0, canvas.w, canvas.h };
    drawBorderedRect(canvas, windowRect, borderR, 3, 0x88333333, 0x88AAAAAA);

    sliderManager.drawSliders(hdc, canvas);
}

void VolumeApp::onResize(RECT rc)
{
    sliderManager.recalculateSliderRects(rc);
    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::onMouseScroll(POINT cursorClientPos, float delta)
{
    auto hoverInfo = sliderManager.getSelectAtPosition(cursorClientPos);
    if (auto slider = sliderManager.getSliderFromSelect(hoverInfo)) {
        float sliderHeight = slider->getHeight();

        float oldVal = pow(slider->_val, .5f);
        float newVal = std::clamp(oldVal + delta / 16, 0.f, 1.f);
        newVal = pow(newVal, 2.f);

        if (newVal != oldVal)
            _audioAppListerner.setVol(hoverInfo, newVal);
    }
}

void VolumeApp::onMouseLeave()
{
    // setWindowSemiTransparent(true);
    if (auto slider = sliderManager.getSliderFromSelect(sliderInfoHovered)) {
        slider->_focused = false;
        InvalidateRect(_hWnd, &slider->_rect, FALSE);
    }
    sliderInfoHovered = {};
}

void VolumeApp::onMouseMove(POINT cursorClientPos, bool justEntered)
{
    // if (justEntered)
    // setWindowSemiTransparent(true);

    SelectInfo newHoverInfo = sliderManager.getSelectAtPosition(cursorClientPos);

    if (newHoverInfo != sliderInfoHovered) {
        if (auto slider = sliderManager.getSliderFromSelect(newHoverInfo)) {
            slider->_focused = true;
            InvalidateRect(_hWnd, &slider->_rect, FALSE);
        }
        if (auto slider = sliderManager.getSliderFromSelect(sliderInfoHovered)) {
            slider->_focused = false;
            InvalidateRect(_hWnd, &slider->_rect, FALSE);
        }
        sliderInfoHovered = newHoverInfo;
    }
}
