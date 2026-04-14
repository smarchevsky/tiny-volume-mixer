#include "Button.h"

#include "Draw.h"
#include "UIManager.h"
#include "Utils.h"

void Button::draw(HDC hdc) const
{
    if (_isPressed) {
        drawGrayscaleMask(hdc, _rleSolid, _imageSize, _pos, nullptr, _color | 0xFF000000);
    } else if (_isHovered) {
        drawGrayscaleMask(hdc, _rleBordered, _imageSize, _pos, nullptr, _color | 0xFF000000);
    } else {
        drawGrayscaleMask(hdc, _rleSolid, _imageSize, _pos, nullptr, _color | _transparency << 24);
    }
}

void Button::initialize(std::vector<DWORD>& pixels, int resourceID, const UIConfig& uic, DWORD color)
{
    SIZE size { 40 - uic.sliderSpacing, 40 - uic.sliderSpacing };
    if (size.cx <= 0 || size.cy <= 0)
        return;

    pixels.resize(size.cx * size.cy);

    auto rect = RECT { 0, 0, size.cx, size.cy };
    drawBorderedRectOverwrite(size, rect, pixels.data(), rect,
        uic.sliderCornerRadius, uic.sliderBorderWidth, 0xFF, 0xFF);
    _rleSolid = PNGLoader::get().createRLEImageMaskFromResource(pixels, resourceID, &size);

    drawBorderedRectOverwrite(size, rect, pixels.data(), rect,
        uic.sliderCornerRadius, uic.sliderBorderWidth, uic.sliderTransparencyBorder, 0xFF);
    _rleBordered = PNGLoader::get().createRLEImageMaskFromResource(pixels, resourceID, &size);

    _color = color & 0xFFFFFF;
    _transparency = uic.sliderTransparencyBorder;

    _imageSize = size;

    _hitExtend[(int)AlignUI::Left] = uic.getSliderOffsetL();
    _hitExtend[(int)AlignUI::Top] = uic.getSliderOffsetL();
    _hitExtend[(int)AlignUI::Right] = uic.getSliderOffsetR();
    _hitExtend[(int)AlignUI::Bottom] = uic.getSliderOffsetR();
}

void Button::setPos(POINT pos, AlignUI align)
{
    switch (align) {
    case AlignUI::LeftTop:
        _pos = pos;
        break;
    case AlignUI::RightTop:
        _pos = { pos.x - _imageSize.cx, pos.y };
        break;
    case AlignUI::LeftBottom:
        _pos = { pos.x, pos.y - _imageSize.cy };
        break;
    case AlignUI::RightBottom:
        _pos = { pos.x - _imageSize.cx, pos.y - _imageSize.cy };
        break;
    }
}

RECT Button::getHitRect() const
{
    return {
        _pos.x - _hitExtend[(int)AlignUI::Left],
        _pos.y - _hitExtend[(int)AlignUI::Top],
        _pos.x + _imageSize.cx + _hitExtend[(int)AlignUI::Right],
        _pos.y + _imageSize.cy + _hitExtend[(int)AlignUI::Bottom]
    };
}

bool Button::onHitEvent(HWND hwnd, HitType type, bool enabled)
{
    if (type == HitType::Hover)
        _isHovered = enabled;
    else if (type == HitType::LMB) {
        _isPressed = enabled;
        if (_onClicked && !_isPressed && _isHovered)
            _onClicked();
    }

    RECT r = getRectDraw();
    InvalidateRect(hwnd, &r, FALSE);
    return true;
}
