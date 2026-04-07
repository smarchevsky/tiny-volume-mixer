#pragma once

#include "Common.h"

void drawBorderedRectAlphaComposite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col);
void drawBorderedRectOverwrite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col);

void drawRoundRectToBitmap(HBITMAP dst, RECT roundRect, int radius, DWORD col0, DWORD col1);

void drawBitmapAlphaComposite(HDC hdc, HBITMAP bmp, const POINT pos, const RECT* customRect, BYTE alpha = 255);
void drawGrayscaleMask(HDC hdc, const ImageBufferRLE img, const POINT pos, const RECT* customRect, DWORD color = 0xFFFFFFFF);
