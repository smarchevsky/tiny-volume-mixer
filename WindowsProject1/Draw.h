#pragma once

#include "Common.h"

void drawBorderedRect(HDC hdc, const RECT rc, int radius, int bw, DWORD bg_col, DWORD bo_col);
void drawStencil(HDC hdc, const StencilGrayscale& stencil, POINT pos, DWORD bg_col);
