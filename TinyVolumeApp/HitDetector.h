#pragma once

#include <Windows.h>

#include "Common.h"

#include <vector>
#include <cassert>

// clang-format off
class IHitTestable {
public:
    virtual void onHoverChanged(HWND hwnd, bool isHover) = 0;
    virtual RECT getHitRect() const = 0;
};

struct HitRect {
    RECT rect;
    IHitTestable* hitPtr;
};

class HitDetector {
    std::vector<HitRect> _hitRects;
    IHitTestable* _currentHit {};

public:
    HWND* _ownerHwnd; // used for invalidate rect

private:
    void onHitChanged(IHitTestable* newHit) {
        assert(_ownerHwnd);
        if (newHit != _currentHit) {
            if (newHit)
                newHit->onHoverChanged(*_ownerHwnd, true);
            if (_currentHit)
                _currentHit->onHoverChanged(*_ownerHwnd, false);
            _currentHit = newHit;
        }
    }
public:
    void addRect(IHitTestable* h) { _hitRects.push_back({ h->getHitRect(), h }); }
    void remove(IHitTestable* h) { std::erase_if(_hitRects, [&](const HitRect& hr) { return hr.hitPtr == h; }); }
    void clear() { _hitRects.clear(); }

    IHitTestable* getCurrentHit() const { return _currentHit; }

    void hitReset() { onHitChanged(nullptr); }
    void hitTest(POINT p)
    {
        IHitTestable* newHit = nullptr;
        for (auto& hr : _hitRects) {
            if (PtInRect(&hr.rect, p)) {
                newHit = hr.hitPtr;
                break;
            }
        }

        onHitChanged(newHit);
    }
};

/*

    if (newHit != _hitHovered) {
        if (newHit == _btnClose._hitUID) {
            _btnClose._isHovered = true;
            RECT bRect = _btnClose.getRectDraw();
            InvalidateRect(_hWnd, &bRect, FALSE);
        }
        if (_hitHovered == _btnClose._hitUID) {
            _btnClose._isHovered = false;
            RECT bRect = _btnClose.getRectDraw();
            InvalidateRect(_hWnd, &bRect, FALSE);
        }


        _hitHovered = newHit;
    }*/
