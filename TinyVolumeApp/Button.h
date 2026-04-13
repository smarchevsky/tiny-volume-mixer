#pragma once

#include "Common.h"

#include "HitDetector.h"

struct UIConfig;

class Button : public IHitTestable {
    BufferRLE _rleSolid;
    BufferRLE _rleBordered;
    DWORD _color;
    BYTE _transparency;
    SIZE _imageSize;
    POINT _pos;
    BYTE _hitExtend[4];

public:
    bool _isPressed, _isHovered;

    SIZE getSize() const { return _imageSize; }
    void setPos(POINT pos, AlignUI align);
    RECT getRectDraw() const { return { _pos.x, _pos.y, _pos.x + _imageSize.cx, _pos.y + _imageSize.cy }; }
    void draw(HDC hdc) const;
    void initialize(std::vector<DWORD>& pixels, int resourceID, const UIConfig& uic);

    RECT getHitRect() const override;
    void onHoverChanged(HWND hwnd, bool isHover) override
    {
        _isHovered = isHover;
        RECT r = getRectDraw();
        InvalidateRect(hwnd, &r, FALSE);
    }

};
