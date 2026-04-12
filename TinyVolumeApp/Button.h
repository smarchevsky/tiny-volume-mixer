#pragma once

#include "Common.h"

struct UIConfig;

class Button {
    BufferRLE _rleSolid;
    BufferRLE _rleBordered;
    DWORD _colorDefault, _colorSemiTransparent;
    SIZE _imageSize;
    POINT _pos;
    BYTE _hitExtend[4];

public:
    HitUID _hitUID;
    enum State : uint8_t {
        Default,
        Hovered,
        Pressed
    } _currentState;

    SIZE getSize() const { return _imageSize; }
    void setPos(POINT pos, AlignUI align);
    RECT getRectHit() const;
    RECT getRectDraw() const { return { _pos.x, _pos.y, _pos.x + _imageSize.cx, _pos.y + _imageSize.cy }; }
    void draw(HDC hdc) const;
    void initialize(std::vector<DWORD>& pixels, int resourceID, const UIConfig& uic);
};
