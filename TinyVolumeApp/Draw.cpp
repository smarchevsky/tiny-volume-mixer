#define NOMINMAX

#include "Draw.h"

#include "ColorUtils.h"
#include "Common.h"
#include "Utils.h"

#include <algorithm>
#include <math.h>

namespace {

__forceinline DWORD setAlpha(DWORD dword, BYTE a) { return dword & 0xFFFFFF | a << 24; }

__forceinline void compositeAlphaInternal(DWORD& back, DWORD front) { back = compositeAlpha(back, front); }

__forceinline void compositeOverwrite(DWORD& back, DWORD front) { back = front; }

__forceinline void replaceRGB(DWORD& back, DWORD front)
{
    DWORD result = lerpColor(back, front, front >> 24);
    back = (back & 0xFF000000) | (result & 0x00FFFFFF);
}

__forceinline void compositeAlphaWithAlpha(DWORD& back, DWORD front, BYTE alpha)
{
    ARGB_SPLIT(BYTE, f, front);
    ARGB_SPLIT(BYTE, b, back);
    fa = fa * alpha / 255;

    if (fa == 255) {
        back = front;
        return;
    }
    if (fa == 0) {
        return;
    }

    BYTE inv_a = 255 - fa;
    back = ARGB(fa + (ba * inv_a) / 255,
        (fr * fa + br * inv_a) / 255,
        (fg * fa + bg * inv_a) / 255,
        (fb * fa + bb * inv_a) / 255);
}

bool validateCommon(HDC hdc, RECT renderableRect, DWORD*& pixels, SIZE& canvasSize, RECT& clipRect)
{
    getBitmapData(getBitmapFromHDC(hdc), canvasSize, pixels);
    if (!pixels)
        return false;

    clipRect = { 0, 0, canvasSize.cx, canvasSize.cy };

    RECT rcClip;
    int regionType = GetClipBox(hdc, &rcClip);
    if (regionType != ERROR && regionType != NULLREGION) {
        clipRect.left = std::max(clipRect.left, rcClip.left);
        clipRect.top = std::max(clipRect.top, rcClip.top);
        clipRect.right = std::min(clipRect.right, rcClip.right);
        clipRect.bottom = std::max(clipRect.bottom, rcClip.bottom);
        // printf("rcClip %d, %d, %d, %d\n", rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
    }

    if (!IntersectRect(&clipRect, &clipRect, &renderableRect))
        return false;

    return true;
}

// template <void (*Op)(DWORD&, DWORD)>
void drawBorderedRectInternal(const SIZE canvasSize, const RECT& clipRegion,
    DWORD* pixels, const RECT roundRect, LONG radius, LONG bw, DWORD bg_col, DWORD bo_col)
{
#define Op compositeOverwrite
    const LONG width = roundRect.right - roundRect.left;
    const LONG height = roundRect.bottom - roundRect.top;

    if (width <= 0 || height <= 0)
        return;

    const LONG half_width = width / 2;
    const LONG half_width1 = (width + 1) / 2;
    const LONG half_height = height / 2;
    const LONG half_height1 = (height + 1) / 2;

    const LONG bwx = std::min(bw, half_width);
    const LONG bwx_right = std::min(bw, half_width1);
    const LONG bwy = std::min(bw, half_height);
    const LONG bwy_bottom = std::min(bw, half_height1);

    const LONG rcl_start = std::max(roundRect.left, clipRegion.left);
    const LONG rct_start = std::max(roundRect.top, clipRegion.top);

    const LONG rct_bwy = roundRect.top + bwy;
    const LONG rct_bwy_start = std::max(rct_bwy, clipRegion.top);
    const LONG rct_bwy_end = std::min(rct_bwy, clipRegion.bottom);

    const LONG rcr_bwx = roundRect.right - bwx;
    const LONG rcr_bwx_start = std::max(rcr_bwx, clipRegion.left);
    const LONG rcr_bwx_end = std::min(rcr_bwx, clipRegion.right);

    const LONG rcb_bwy = roundRect.bottom - bwy_bottom;
    const LONG rcb_bwy_start = std::max(rcb_bwy, clipRegion.top);
    const LONG rcb_bwy_end = std::min(rcb_bwy, clipRegion.bottom);

    const LONG rcl_bwx = roundRect.left + bwx;
    const LONG rcl_bwx_start = std::max(rcl_bwx, clipRegion.left);
    const LONG rcl_bwx_end = std::min(rcl_bwx, clipRegion.right);

    const LONG rcb_end = std::min(roundRect.bottom, clipRegion.bottom);
    const LONG rcr_end = std::min(roundRect.right, clipRegion.right);

    if (bg_col != bo_col) {
        if (rcl_bwx_start < rcr_bwx_end) {
            for (LONG y = rct_start; y < rct_bwy_end; y++) // top border
                for (LONG x = rcl_bwx_start; x < rcr_bwx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bo_col);

            for (LONG y = rct_bwy_start; y < rct_bwy_end; y++) // top section
                for (LONG x = rcl_bwx_start; x < rcr_bwx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bg_col);

            for (LONG y = rcb_bwy_start; y < rcb_bwy_end; y++) // bottom section
                for (LONG x = rcl_bwx_start; x < rcr_bwx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bg_col);

            for (LONG y = rcb_bwy_start; y < rcb_end; y++) // bottom border
                for (LONG x = rcl_bwx_start; x < rcr_bwx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bo_col);
        }

        if (rcl_bwx_start < rcr_bwx_end) {
            for (LONG y = rct_bwy_start; y < rcb_bwy_end; y++) // mid section
                for (LONG x = rcl_bwx_start; x < rcr_bwx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bg_col);
        }

        if (rcl_start < rcl_bwx_end) {
            for (LONG y = rct_bwy_start; y < rcb_bwy_end; y++) // left border
                for (LONG x = rcl_start; x < rcl_bwx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bo_col);
        }

        if (rcr_bwx_start < rcr_end) {
            for (LONG y = rct_bwy_start; y < rcb_bwy_end; y++) // right border
                for (LONG x = rcr_bwx_start; x < rcr_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bo_col);
        }

    } else {
        for (LONG y = rct_start; y < rct_bwy_end; y++) // top section & border
            for (LONG x = rcl_bwx_start; x < rcr_bwx_end; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);

        for (LONG y = rcb_bwy_start; y < rcb_end; y++) // bottom section & border
            for (LONG x = rcl_bwx_start; x < rcr_bwx_end; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);

        if (rcl_start < rcr_end) {
            for (LONG y = rct_bwy_start; y < rcb_bwy_end; y++) //  left & right & mid section
                for (LONG x = rcl_start; x < rcr_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bg_col);
        }
    }

    // return;

    // corners
    auto makeDist = [](LONG x, LONG y, LONG r) {
        float fx = float(r - x), fy = float(r - y);
        return r - sqrtf(fx * fx + fy * fy);
    };

    auto CompositeRadius = [&](DWORD& back, float dist) {
        dist += 1;
        float t = std::clamp(dist - bw, 0.f, 1.f);
        float a = std::clamp(dist, 0.f, 1.f);

        if (a > 0.f) {
            DWORD col = lerpColor(bo_col, bg_col, BYTE(t * 255));
            // OVERWRITE
            back = lerpColor(back, col, BYTE(a * 255));

            // ALPHA BLEND
            // col = setAlpha(col, BYTE((col >> 24) * a));
            // Op(back, col);
        }
    };

    const LONG minrx = std::min(radius, half_width);
    const LONG minrx_right = std::min(radius, half_width1);
    const LONG minry = std::min(radius, half_height);
    const LONG minry_bottom = std::min(radius, half_height1);

    const LONG rcb_minry = roundRect.bottom - minry_bottom;
    const LONG rcb_minry_start = std::max(rcb_minry, clipRegion.top);
    const LONG rcb_minry_end = std::min(rcb_minry, clipRegion.bottom);

    const LONG rct_minry = roundRect.top + minry;
    const LONG rct_minry_start = std::max(rct_minry, clipRegion.top);
    const LONG rct_minry_end = std::min(rct_minry, clipRegion.bottom);

    const LONG rcl_minrx_start = std::max(roundRect.left + minrx, clipRegion.left);
    const LONG rcl_minrx_end = std::min(roundRect.left + minrx_right, clipRegion.right);

    const LONG rcr_minrx_start = std::max(roundRect.right - minrx, clipRegion.left);
    const LONG rcr_minrx_end = std::min(roundRect.right - minrx_right, clipRegion.right);

    if (rcl_start < rcl_minrx_end) {
        for (LONG y = rct_start; y < rct_minry_end; y++) // top left
            for (LONG x = rcl_start; x < rcl_minrx_end; x++) {
                float dist = makeDist(x - roundRect.left, y - roundRect.top, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }

        for (LONG y = rcb_minry_start; y < rcb_end; y++) // bottom left
            for (LONG x = rcl_start; x < rcl_minrx_end; x++) {
                float dist = makeDist(x - roundRect.left, height - y - 1 + roundRect.top, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }
    }

    if (rcr_minrx_start < rcr_end) {
        for (LONG y = rct_start; y < rct_minry_end; y++) // top right
            for (LONG x = rcr_minrx_start; x < rcr_end; x++) {
                float dist = makeDist(width - x - 1 + roundRect.left, y - roundRect.top, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }

        for (LONG y = rcb_minry_start; y < rcb_end; y++) // bottom right
            for (LONG x = rcr_minrx_start; x < rcr_end; x++) {
                float dist = makeDist(width - x - 1 + roundRect.left, height - y - 1 + roundRect.top, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }
    }
}
}

void drawBorderedRectAlphaComposite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col)
{
    DWORD* pixels {};
    SIZE canvasSize;
    RECT clipRect;
    if (!validateCommon(hdc, roundRect, pixels, canvasSize, clipRect))
        return;

    // <compositeAlphaInternal>
    drawBorderedRectInternal(canvasSize, clipRect, pixels, roundRect, radius, bw, bg_col, bo_col);
}

void drawBorderedRectOverwrite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col)
{
    DWORD* pixels {};
    SIZE canvasSize;
    RECT clipRect;
    if (!validateCommon(hdc, roundRect, pixels, canvasSize, clipRect))
        return;

    // <compositeOverwrite>
    drawBorderedRectInternal(canvasSize, clipRect, pixels, roundRect, radius, bw, bg_col, bo_col);
}

void drawBitmapAlphaComposite(HDC hdc, HBITMAP bmp, const POINT pos, const RECT* customRect, BYTE alpha)
{
    if (!bmp)
        return;

    SIZE bitmapSize;
    DWORD* bitmapPixels;
    getBitmapData(bmp, bitmapSize, bitmapPixels);
    RECT bitmapRect = { pos.x, pos.y, pos.x + bitmapSize.cx, pos.y + bitmapSize.cy };

    SIZE canvasSize;
    RECT clipRect;
    DWORD* canvasPixels;
    if (!validateCommon(hdc, bitmapRect, canvasPixels, canvasSize, clipRect))
        return;

    if (customRect) {
        if (!IntersectRect(&bitmapRect, &bitmapRect, customRect))
            return;
    }

    for (int y = clipRect.top; y < clipRect.bottom; y++) {
        int hdcY = y * canvasSize.cx;
        int stencilY = (y - pos.y) * bitmapSize.cx - pos.x;

        for (int x = clipRect.left; x < clipRect.right; x++) {
            DWORD front = bitmapPixels[stencilY + x];
            DWORD& back = canvasPixels[hdcY + x];
            compositeAlphaWithAlpha(back, front, alpha);
        }
    }
}

void drawRoundRectToBitmap(HBITMAP dst, RECT roundRect, int radius, DWORD col0, DWORD col1)
{
    SIZE bitmapSize;
    DWORD* pixels;
    getBitmapData(dst, bitmapSize, pixels);
    if (!pixels)
        return;

    for (int i = 0; i < bitmapSize.cx * bitmapSize.cy; ++i)
        pixels[i] = (pixels[i] & 0xFF000000) | (col0 & 0x00FFFFFF);

    // <replaceRGB>
    drawBorderedRectInternal(bitmapSize, { 0, 0, bitmapSize.cx, bitmapSize.cy },
        pixels, roundRect, radius, 0, 0xFF000000 | col1, 0xFF000000 | col1);
}
