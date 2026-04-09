#include "VolumeApp.h"

#include "ColorUtils.h"
#include "UIManager.h"
#include "Utils.h"

#include "resource.h"

ImageBufferRLE imageClose, imageSettings;

void VolumeApp::construct(HINSTANCE instance, WNDPROC wndProc)
{
    RECT rc;
    FileManager::get().loadWindowRect(rc);

    UIManager::get().init(_uic);

    initWindow(instance, wndProc, rc);
    // _isAppHovered = false, updateTimerStateUI();

    _sliderManager.getSliderFromSelect(SelectInfo(VolumeType::Master, 0))->_sliderInfo
        = UIManager::get().getIconMasterVol();

    _audioAppListerner.init(_hWnd);

    int customSize { 40 - _uic.sliderSpacing };
    imageClose = PNGLoader::get().getGrayscalePngFromResource(IDB_PNG_CLOSE, &customSize);
    imageSettings = PNGLoader::get().getGrayscalePngFromResource(IDB_PNG_SETTINGS, &customSize);
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

static void startTimer(HWND hwnd) { SetTimer(hwnd, WM_TIMER_UI, 100, (TIMERPROC)NULL), printf("Timer start\n"); }
static void stopTimer(HWND hwnd) { KillTimer(hwnd, WM_TIMER_UI), printf("Timer stop\n"); };

void VolumeApp::handleMMAppRegistered(AudioSessionInitInfo* sessionInitInfo)
{
    AudioUpdateInfo& info = sessionInitInfo->updateInfo;
    SliderInfo* iconInfo = UIManager::get().generateSliderInfo(sessionInitInfo->iconPath, info._pid);
    _sliderManager.appSliderAdd(info._pid, info._vol, info._isMuted, iconInfo);

    if (sessionInitInfo->audioSessionState == AudioSessionState::AudioSessionStateActive)
        startTimer(_hWnd);

    delete sessionInitInfo;

    RECT rc;
    GetClientRect(_hWnd, &rc);
    _sliderManager.recalculateSliderRects(rc, _uic);

    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::handleMMAppUnegistered(WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(_hWnd, &rc);

    AudioUpdateInfo info(wParam, lParam);
    _sliderManager.appSliderRemove(info._pid);
    _sliderManager.recalculateSliderRects(rc, _uic);

    InvalidateRect(_hWnd, NULL, FALSE);
    _audioAppListerner.cleanupExpiredSessions();
}

void VolumeApp::handleMMAppActivationChanged(WPARAM wParam, LPARAM lParam)
{
    auto info = reinterpret_cast<ActivationChangedInfo&>(wParam);

    if (Slider* slider = _sliderManager.getSliderFromSelect(SelectInfo(VolumeType::App, info.pid))) {
        slider->_peak = 0.f;
        InvalidateRect(_hWnd, &slider->_rect, FALSE);
    }

    if (info.active)
        startTimer(_hWnd);
}

void VolumeApp::handleMMRefreshVol(WPARAM wParam, LPARAM lParam)
{
    AudioUpdateInfo info(wParam, lParam);
    SelectInfo si(info._type, info._pid);
    if (auto slider = _sliderManager.getSliderFromSelect(si)) {
        slider->_val = info._vol;
        RECT u = slider->calculateTextRect();
        UnionRect(&u, &u, &slider->_rect);
        InvalidateRect(_hWnd, &u, FALSE);
    }
}

void VolumeApp::onPaint(HDC hdc)
{
    RECT windowRect;
    GetClientRect(_hWnd, &windowRect);

    DWORD bg_col = _uic.windowBackgroundRGBA;
    DWORD bo_col = _uic.windowBorderRGBA;

#define OVERWRITE 1
#if OVERWRITE == 1
    bg_col = compositeAlpha(0, bg_col);
    bo_col = compositeAlpha(0, bo_col);
    drawBorderedRectOverwrite(hdc, windowRect,
        _uic.windowCornerRadius, _uic.windowBorderWidth,
        bg_col, bo_col);
#else
    drawBorderedRectAlphaComposite(hdc, windowRect,
        _uic.windowCornerRadius, _uic.windowBorderWidth,
        bg_col, bo_col);
#endif

    _sliderManager.drawSliders(hdc, _uic);

    // POINT p;
    // GetCursorPos(&p);
    // ScreenToClient(_hWnd, &p);

    int bd = _uic.sliderSpacing + _uic.windowBorderWidth;
    drawGrayscaleMask(hdc, imageClose,
        POINT { windowRect.right - imageClose.w - bd, bd },
        nullptr, 0x88AA0033);
    drawGrayscaleMask(hdc, imageSettings,
        POINT { windowRect.right - imageSettings.w - bd, imageSettings.h + _uic.sliderSpacing + bd },
        nullptr, 0x88888888);

    // overlay text
    if (auto slider = _sliderManager.getSliderFromSelect(_sliderInfoHovered)) {
        if (slider && slider->_sliderInfo && slider->_sliderInfo->textBmp) {
            RECT r = slider->calculateTextRect();
            drawBitmapAlphaComposite(hdc, slider->_sliderInfo->textBmp,
                { r.left, r.top }, nullptr, 180);
        }
    }
}

void VolumeApp::onResize(RECT rc)
{
    _sliderManager.recalculateSliderRects(rc, _uic);
    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::onMouseScroll(POINT cursorClientPos, float delta)
{
    auto hoverInfo = _sliderManager.getSelectAtPosition(cursorClientPos);
    if (auto slider = _sliderManager.getSliderFromSelect(hoverInfo)) {
        float sliderHeight = slider->getHeight();
        // slider->debugUpdateIcon(_uic.iconSize);

        float oldVal = powf(slider->_val, .5f);
        float newVal = std::clamp(oldVal + delta / 16, 0.f, 1.f);
        newVal = powf(newVal, 2.f);
        if (newVal != oldVal)
            _audioAppListerner.setVol(hoverInfo, newVal);
    }
}

void VolumeApp::onMouseLeave()
{
    // _isAppHovered = false, updateTimerStateUI();

    if (auto slider = _sliderManager.getSliderFromSelect(_sliderInfoHovered)) {
        slider->_focused = false;
        RECT u = slider->calculateTextRect();
        UnionRect(&u, &u, &slider->_rect);
        InvalidateRect(_hWnd, &u, FALSE);
    }
    _sliderInfoHovered = {};
}

void VolumeApp::onMouseMove(POINT cursorClientPos, bool justEntered)
{
    // if (justEntered)
    //     _isAppHovered = true, updateTimerStateUI();

    SelectInfo newHoverInfo = _sliderManager.getSelectAtPosition(cursorClientPos);

    if (newHoverInfo != _sliderInfoHovered) {
        if (auto slider = _sliderManager.getSliderFromSelect(newHoverInfo)) {
            slider->_focused = true;
            RECT u = slider->calculateTextRect();
            UnionRect(&u, &u, &slider->_rect);
            InvalidateRect(_hWnd, &u, FALSE);
        }
        if (auto slider = _sliderManager.getSliderFromSelect(_sliderInfoHovered)) {
            slider->_focused = false;
            RECT u = slider->calculateTextRect();
            UnionRect(&u, &u, &slider->_rect);
            InvalidateRect(_hWnd, &u, FALSE);
        }
        _sliderInfoHovered = newHoverInfo;
    }
}

void VolumeApp::handleTimerUpdateUI()
{
    // std::string str;
    float masterPeak = 0.f;
    std::vector<WaveInfo> waveInfos;
    bool anyActive = _audioAppListerner.retieveWaveInfo(waveInfos, masterPeak);

    Slider* masterSlider = _sliderManager.getSliderFromSelect(SelectInfo(VolumeType::Master, 0));

    if (!anyActive) {
        _sliderManager.forEachSliderApp([&](Slider& slider) {
            if (slider._peak != 0.f)
                slider._peak = 0.f, InvalidateRect(_hWnd, &slider._rect, FALSE);
        });

        if (masterSlider->_peak != 0.f)
            masterSlider->_peak = 0.f, InvalidateRect(_hWnd, &masterSlider->_rect, FALSE);

        stopTimer(_hWnd);
        return;
    }

    if (masterSlider->_peak != masterPeak)
        masterSlider->_peak = masterPeak, InvalidateRect(_hWnd, &masterSlider->_rect, FALSE);

    for (auto& w : waveInfos) {
        if (Slider* slider = _sliderManager.getSliderFromSelect(SelectInfo(VolumeType::App, w.pid))) {
            if (slider->_peak != w.wave)
                slider->_peak = w.wave, InvalidateRect(_hWnd, &slider->_rect, FALSE);
        }
        // str += "PID: " + std::to_string((int)w.pid) + ", Wave: " + std::to_string(w.wave) + "   ";
    }
    // printf("Wave info:\n%s", str.c_str());
}