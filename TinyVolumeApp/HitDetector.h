#pragma once

#include <Windows.h>

#include "Common.h"

#include <vector>

struct HitRect {
    RECT rect;
    HitUID ownerId;
};

class HitDetector {
    std::vector<HitRect> _hitRects;
    HitUID _UID = 0;

public:
    HitUID addRect(RECT rect)
    {
        _hitRects.push_back({ rect, ++_UID });
        return _UID;
    }

    void remove(HitUID hitUID)
    {
        std::erase_if(_hitRects, [&](const HitRect& hr) { return hr.ownerId == hitUID; });
    }

    void clear() { _hitRects.clear(), _UID = 0; }

    HitUID testHit(POINT p)
    {
        for (auto& hr : _hitRects) {
            if (PtInRect(&hr.rect, p)) {
                return hr.ownerId;
            }
        }

        return HitUID_none;
    }
};
