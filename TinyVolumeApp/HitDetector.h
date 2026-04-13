#pragma once

#include <Windows.h>

#include "Common.h"

#include <cassert>
#include <vector>

// clang-format off

enum HitType : uint8_t { Hover, LMB, RMB, MMB, Max };

class IHitTestable {
public:
    virtual void onHitEvent(HWND hwnd, HitType type, bool enabled) = 0;
    virtual RECT getHitRect() const = 0;
};

struct HitRect {
    RECT rect;
    IHitTestable* hitPtr;
};

class HitDetector {
    std::vector<HitRect> _hitRects;
    IHitTestable* _currentHit [HitType::Max] {};

private:
    void onHitChanged(HWND hwnd, IHitTestable* newHover, HitType type) {
        if (newHover != _currentHit[type]) {
            if (newHover)
                newHover->onHitEvent(hwnd, type, true);
            if (_currentHit[type])
                _currentHit[type]->onHitEvent(hwnd, type, false);
            _currentHit[type] = newHover;
        }
    }
public:
    void addRect(IHitTestable* h) { _hitRects.push_back({ h->getHitRect(), h }); }
    void remove(IHitTestable* h) { std::erase_if(_hitRects, [&](const HitRect& hr) { return hr.hitPtr == h; }); }
    void clear() { _hitRects.clear(); }

    IHitTestable* getCurrentHit(HitType type) const { return _currentHit[type]; }

    void hitReset(HWND hwnd, HitType type) { onHitChanged(hwnd, nullptr, type); }
    bool hitTest(HWND hwnd, POINT p, HitType type)
    {
        IHitTestable* newHit = nullptr;
        for (auto& hr : _hitRects) {
            if (PtInRect(&hr.rect, p)) {
                newHit = hr.hitPtr;
                break;
            }
        }

        onHitChanged(hwnd, newHit, type);

        return !!newHit;
    }
};