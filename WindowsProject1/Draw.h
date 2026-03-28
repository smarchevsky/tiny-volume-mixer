#pragma once

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>

void drawBorderedRect(HDC hdc, const RECT rc, int radius, int bw, DWORD bg_col, DWORD bo_col);