#pragma once

#include "Common.h"

void drawBorderedRect(HDC hdc, const RECT rc, int radius, int bw, DWORD bg_col, DWORD bo_col);
void drawStencil(HDC hdc, BYTE* data, const SIZE stencilSize, const POINT pos, DWORD col0, DWORD col1, int horizontalShift = 0);
