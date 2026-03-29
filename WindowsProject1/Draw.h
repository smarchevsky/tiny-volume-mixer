#pragma once

#include "Common.h"

void drawBorderedRectAlphaComposite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col);
void drawBitmapAlphaComposite(HDC hdc, HBITMAP bmp, const POINT pos, int horizontalShift = 0);
void drawRoundRectToBitmap(HBITMAP dst, const POINT pos, RECT roundRect, int radius, DWORD col0, DWORD col1);
