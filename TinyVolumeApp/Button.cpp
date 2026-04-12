#include "Button.h"

#include "Draw.h"
#include "UIManager.h"
#include "Utils.h"

void Button::draw(HDC hdc) const
{
    auto& currentRLE = _currentState == Hovered ? _rleBordered : _rleSolid;
    auto& currentColor = _currentState == Default ? _colorSemiTransparent : _colorDefault;
    drawGrayscaleMask(hdc, currentRLE, _imageSize, _pos, nullptr, currentColor);
}

void Button::initialize(std::vector<DWORD>& pixels, int resourceID, const UIConfig& uic)
{
    SIZE size { 40 - uic.sliderSpacing, 40 - uic.sliderSpacing };
    if (size.cx <= 0 || size.cy <= 0)
        return;

    pixels.resize(size.cx * size.cy);

    BYTE semiTransparent = uic.sliderTransparencyBorder;

    auto rect = RECT { 0, 0, size.cx, size.cy };
    drawBorderedRectOverwrite(size, rect, pixels.data(), rect, uic.sliderCornerRadius, uic.sliderBorderWidth, 0xFF, 0xFF);
    _rleSolid = PNGLoader::get().createRLEImageMaskFromResource(pixels, resourceID, &size);

    drawBorderedRectOverwrite(size, rect, pixels.data(), rect, uic.sliderCornerRadius, uic.sliderBorderWidth, semiTransparent, 0xFF);
    _rleBordered = PNGLoader::get().createRLEImageMaskFromResource(pixels, resourceID, &size);

    _colorDefault = 0xFFAA0033;
    _colorSemiTransparent = 0x00AA0033 | semiTransparent << 24;

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

RECT Button::getRectHit() const
{
    return {
        _pos.x - _hitExtend[(int)AlignUI::Left],
        _pos.y - _hitExtend[(int)AlignUI::Top],
        _pos.x + _imageSize.cx + _hitExtend[(int)AlignUI::Right],
        _pos.y + _imageSize.cy + _hitExtend[(int)AlignUI::Bottom]
    };
}