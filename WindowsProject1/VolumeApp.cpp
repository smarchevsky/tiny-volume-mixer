#include "VolumeApp.h"

#include "IconManager.h"
#include "Utils.h"

static PixelBuffer_RG stencil;

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

    TextRenderer::get().init();
    stencil = TextRenderer::get().renderTextToGrayscaleStencil(L"Hello, world!");
    // auto s = stencil.getSize();
    // auto data = stencil.data();
    // printf("stencil size %d %d\n", s.cx, s.cy);
    // for (int i = 0; i < s.cx * s.cy; ++i) {
    //     printf("%d ", data[i]);
    // }
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
    RECT windowRect;
    GetClientRect(_hWnd, &windowRect);
    drawBorderedRect(hdc, windowRect, _uiConfig.frameCornerRadius, _uiConfig.frameBorderWidth, 0x88333333, 0x88AAAAAA);
    sliderManager.drawSliders(hdc, _uiConfig);

    drawStencil(hdc, stencil.data<0>(), stencil.size(), { 400, 200 }, 0x0000FF00, 0xFF00FF00);
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
