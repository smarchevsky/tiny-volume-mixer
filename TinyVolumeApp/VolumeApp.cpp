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
    handleHoverChanged(false);

    _sliderManager.getSliderMaster()._sliderInfo = UIManager::get().getIconMasterVol();

    _audioAppListerner.init(_hWnd);

    SIZE customSize { 40 - _uic.sliderSpacing, 40 - _uic.sliderSpacing };
    int r = _uic.sliderCornerRadius;
    int bw = _uic.sliderBorderWidth;
    imageClose = PNGLoader::get().getGrayscalePngFromResource(IDB_PNG_CLOSE, r, bw, &customSize);
    imageSettings = PNGLoader::get().getGrayscalePngFromResource(IDB_PNG_SETTINGS, r, bw, &customSize);
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
    recalculateHitRects(rc);
    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::handleMMAppUnegistered(WPARAM wParam, LPARAM lParam)
{
    RECT rc;
    GetClientRect(_hWnd, &rc);

    AudioUpdateInfo info(wParam, lParam);
    _sliderManager.appSliderRemove(info._pid);
    recalculateHitRects(rc);
    InvalidateRect(_hWnd, NULL, FALSE);
    _audioAppListerner.cleanupExpiredSessions();
}

void VolumeApp::handleMMAppActivationChanged(WPARAM wParam, LPARAM lParam)
{
    auto info = reinterpret_cast<ActivationChangedInfo&>(wParam);

    if (Slider* slider = _sliderManager.getSliderAppByPID(info.pid)) {
        slider->_peak = 0.f;
        InvalidateRect(_hWnd, &slider->_rect, FALSE);
    }

    if (info.active)
        startTimer(_hWnd);
}

void VolumeApp::handleMMRefreshVol(WPARAM wParam, LPARAM lParam)
{
    AudioUpdateInfo info(wParam, lParam);
    Slider* slider = {};
    if (info._type == VolumeType::Master)
        slider = &_sliderManager.getSliderMaster();
    else if (info._type == VolumeType::App)
        slider = _sliderManager.getSliderAppByPID(info._pid);

    if (slider) {
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

    if (_isAppHovered) {
        int bd = _uic.sliderSpacing + _uic.windowBorderWidth;
        drawGrayscaleMask(hdc, imageClose,
            POINT { windowRect.right - imageClose.w - bd, bd },
            nullptr, 0x88AA0033);
        drawGrayscaleMask(hdc, imageSettings,
            POINT { windowRect.right - imageSettings.w - bd, imageSettings.h + _uic.sliderSpacing + bd },
            nullptr, 0x88888888);
    }

    // overlay text
    if (auto slider = _sliderManager.getSliderFromHitUID(_hitHovered)) {
        if (slider && slider->_sliderInfo && slider->_sliderInfo->textBmp) {
            RECT r = slider->calculateTextRect();
            drawBitmapAlphaComposite(hdc, slider->_sliderInfo->textBmp,
                { r.left, r.top }, nullptr, 180);
        }
    }
}

void VolumeApp::onResize(RECT rc)
{
    recalculateHitRects(rc);
    InvalidateRect(_hWnd, NULL, FALSE);
}

void VolumeApp::onMouseScroll(POINT cursorClientPos, float delta)
{
    if (Slider* slider = _sliderManager.getSliderFromHitUID(_hitHovered)) {
        float sliderHeight = slider->getHeight();
        // slider->debugUpdateIcon(_uic.iconSize);

        float oldVal = powf(slider->_val, .5f);
        float newVal = std::clamp(oldVal + delta / 16, 0.f, 1.f);
        newVal = powf(newVal, 2.f);
        if (newVal != oldVal) {
            if (slider == &_sliderManager.getSliderMaster())
                _audioAppListerner.setVolMaster(newVal);
            else
                _audioAppListerner.setVolApp(slider->getPID(), newVal);
        }
    }
}

void VolumeApp::recalculateHitRects(const RECT& rc)
{
    _hitDetector.clear();
    _sliderManager.recalculateSliderRects(rc, _uic);
    _sliderManager.getSliderMaster()._hitUID = _hitDetector.addRect(_sliderManager.getSliderMaster()._rect);
    _sliderManager.forEachSliderApp([&](Slider& s) { s._hitUID = _hitDetector.addRect(s._rect); });
}

void VolumeApp::onMouseLeave()
{
    handleHoverChanged(false);

    if (auto slider = _sliderManager.getSliderFromHitUID(_hitHovered)) {
        slider->_focused = false;
        RECT u = slider->calculateTextRect();
        UnionRect(&u, &u, &slider->_rect);
        InvalidateRect(_hWnd, &u, FALSE);
    }

    _hitHovered = HitUID_none;
}

void VolumeApp::onMouseMove(POINT cursorClientPos, bool justEntered)
{
    if (justEntered)
        handleHoverChanged(true);

    HitUID newHit = _hitDetector.testHit(cursorClientPos);

    if (newHit != _hitHovered) {
        if (auto slider = _sliderManager.getSliderFromHitUID(newHit)) {
            slider->_focused = true;
            RECT u = slider->calculateTextRect();
            UnionRect(&u, &u, &slider->_rect);
            InvalidateRect(_hWnd, &u, FALSE);
        }
        if (auto slider = _sliderManager.getSliderFromHitUID(_hitHovered)) {
            slider->_focused = false;
            RECT u = slider->calculateTextRect();
            UnionRect(&u, &u, &slider->_rect);
            InvalidateRect(_hWnd, &u, FALSE);
        }

        _hitHovered = newHit;
    }
}

void VolumeApp::handleTimerUpdateUI()
{
    // std::string str;
    float masterPeak = 0.f;
    std::vector<WaveInfo> waveInfos;
    bool anyActive = _audioAppListerner.retieveWaveInfo(waveInfos, masterPeak);

    Slider& masterSlider = _sliderManager.getSliderMaster();

    if (!anyActive) {
        _sliderManager.forEachSliderApp([&](Slider& slider) {
            if (slider._peak != 0.f)
                slider._peak = 0.f, InvalidateRect(_hWnd, &slider._rect, FALSE);
        });

        if (masterSlider._peak != 0.f)
            masterSlider._peak = 0.f, InvalidateRect(_hWnd, &masterSlider._rect, FALSE);

        stopTimer(_hWnd);
        return;
    }

    if (masterSlider._peak != masterPeak)
        masterSlider._peak = masterPeak, InvalidateRect(_hWnd, &masterSlider._rect, FALSE);

    for (auto& w : waveInfos) {
        if (Slider* slider = _sliderManager.getSliderAppByPID(w.pid)) {
            if (slider->_peak != w.wave)
                slider->_peak = w.wave, InvalidateRect(_hWnd, &slider->_rect, FALSE);
        }
        // str += "PID: " + std::to_string((int)w.pid) + ", Wave: " + std::to_string(w.wave) + "   ";
    }
    // printf("Wave info:\n%s", str.c_str());
}

bool VolumeApp::handleHoverChanged(bool isHovered)
{
    _isAppHovered = isHovered;
    InvalidateRect(_hWnd, NULL, FALSE);
    return false;
}
