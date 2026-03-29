#pragma once

#include "Common.h"

void drawBorderedRectAlphaComposite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col);
void drawBitmapAlphaComposite(HDC hdc, HBITMAP bmp, const POINT pos, int horizontalShift = 0);
