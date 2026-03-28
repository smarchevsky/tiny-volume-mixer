#include "VolumeApp.h"

#include "IconManager.h"
#include "Utils.h"

void VolumeApp::construct(HINSTANCE instance, WNDPROC wndProc)
{
    RECT rc;
    FileManager::get().loadWindowRect(rc);

    initWindow(instance, wndProc, rc);
    // setWindowSemiTransparent(true);

    _uiConfig = UIConfig::get();

    IconManager::get().init(_uiConfig.iconSize);
    sliderManager.getSliderFromSelect(SelectInfo(VolumeType::Master, 0))->_iconInfo
        = IconManager::get().getIconMasterVol();

    _audioAppListerner.init(_hWnd);

    _hFont = CreateFont(
        28, // Height (arbitrary size)
        0, // Width (0 let's Windows choose best match)
        0, // Escapement
        0, // Orientation
        FW_BOLD, // Weight (e.g., FW_NORMAL, FW_BOLD)
        FALSE, // Italic
        FALSE, // Underline
        FALSE, // Strikeout
        ANSI_CHARSET, // Charset
        OUT_DEFAULT_PRECIS, // Out Precision
        CLIP_DEFAULT_PRECIS, // Clip Precision
        DEFAULT_QUALITY, // Quality
        DEFAULT_PITCH | FF_SWISS, // Pitch and Family
        L"Segoe UI" // Typeface name
    );

    HDC hdcScreen = GetDC(NULL);
    _fontDC = CreateCompatibleDC(hdcScreen);
    ReleaseDC(NULL, hdcScreen);

    SelectObject(_fontDC, _hFont);

    std::wstring text = L"Firefox";
    SIZE size;
    GetTextExtentPoint32(_fontDC, text.c_str(), (int)text.length(), &size);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size.cx;
    bmi.bmiHeader.biHeight = -size.cy; // Negative for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; // 32-bit for Alpha support
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits;
    HBITMAP hBitmap = CreateDIBSection(_fontDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    SelectObject(_fontDC, hBitmap);

    memset(pvBits, 0, size.cx * size.cy * 4);

    SetBkMode(_fontDC, TRANSPARENT);
    SetTextColor(_fontDC, RGB(255, 255, 255));

    RECT rect = { 0, 0, size.cx, size.cy };
    TextOut(_fontDC, 0, 0, text.c_str(), (int)text.length());
    _textSize = { size.cx, size.cy };
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

    IconInfo* iconInfo = IconManager::get().tryRetrieveIcon(sessionInitInfo->iconPath, info._pid);

    sliderManager.appSliderAdd(info._pid, info._vol, info._isMuted, iconInfo);
    delete sessionInitInfo;

    RECT rc;
    GetClientRect(_hWnd, &rc);
    sliderManager.recalculateSliderRects(rc, _uiConfig);

    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::handleMMAppUnegistered(WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(_hWnd, &rc);

    AudioUpdateInfo info(wParam, lParam);
    sliderManager.appSliderRemove(info._pid);
    sliderManager.recalculateSliderRects(rc, _uiConfig);

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

void VolumeApp::onPaint(HDC hdc)
{
    BitBlt(hdc, 194, 16, _textSize.cx, _textSize.cy, _fontDC, 0, 0, SRCCOPY);

    RECT windowRect;
    GetClientRect(_hWnd, &windowRect);
    drawBorderedRect(hdc, windowRect, _uiConfig.frameCornerRadius, _uiConfig.frameBorderWidth, 0x88333333, 0x88AAAAAA);
    sliderManager.drawSliders(hdc, _uiConfig);
}

void VolumeApp::onResize(RECT rc)
{
    sliderManager.recalculateSliderRects(rc, _uiConfig);
    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::onMouseScroll(POINT cursorClientPos, float delta)
{
    auto hoverInfo = sliderManager.getSelectAtPosition(cursorClientPos);
    if (auto slider = sliderManager.getSliderFromSelect(hoverInfo)) {
        float sliderHeight = slider->getHeight();
        // slider->debugUpdateIcon(_uiConfig.iconSize);

        float oldVal = powf(slider->_val, .5f);
        float newVal = std::clamp(oldVal + delta / 16, 0.f, 1.f);
        newVal = powf(newVal, 2.f);
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
