#pragma once

#include <Windows.h>

#include <vector>

typedef uint32_t HitUID;
constexpr HitUID HitUID_invalid = HitUID(-1);

enum class HitGroupType : uint8_t {
    Slider,
    Button
};

struct HitRect {
    RECT rect;
    HitUID ownerId;
    HitGroupType groupID;
};

class HitDetector {
    std::vector<HitRect> _hitRects;
    HitUID _UID = 0;

public:
    HitUID addRect(RECT rect, HitGroupType groupID)
    {
        _hitRects.push_back({ rect, ++_UID, groupID });
        return _UID;
    }

    void remove(HitUID hitUID)
    {
        std::erase_if(_hitRects, [&](const HitRect& hr) { return hr.ownerId == hitUID; });
    }

    void removeGroup(HitGroupType groupID)
    {
        std::erase_if(_hitRects, [groupID](const HitRect& hr) { return hr.groupID == groupID; });
    }

    HitUID testHit(POINT p)
    {
        for (auto& hr : _hitRects) {
            if (PtInRect(&hr.rect, p)) {
                return hr.ownerId;
            }
        }

        return HitUID_invalid;
    }
};
