#define NOMINMAX

#include "Draw.h"

#include "ColorUtils.h"
#include "Common.h"
#include "Utils.h"

#include <algorithm>
#include <cassert>
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

template <void (*Op)(DWORD&, DWORD)>
void drawBorderedRectInternal(const SIZE canvasSize, const RECT& clipRegion,
    DWORD* pixels, const RECT roundRect, LONG radius, LONG bw, DWORD bg_col, DWORD bo_col)
{
    using std::clamp;
    using std::max;
    using std::min;

    const LONG width = roundRect.right - roundRect.left;
    const LONG height = roundRect.bottom - roundRect.top;

    if (width <= 0 || height <= 0)
        return;

    const LONG left = max(roundRect.left, clipRegion.left);
    const LONG top = max(roundRect.top, clipRegion.top);
    const LONG right = min(roundRect.right, clipRegion.right);
    const LONG bottom = min(roundRect.bottom, clipRegion.bottom);

    const LONG half_width = width / 2;
    const LONG half_width1 = (width + 1) / 2;
    const LONG half_height = height / 2;
    const LONG half_height1 = (height + 1) / 2;

    const LONG left_border = max(roundRect.left + min(bw, half_width), clipRegion.left);
    const LONG top_border = max(roundRect.top + min(bw, half_height), clipRegion.top);
    const LONG right_border = min(roundRect.right - min(bw, half_width1), clipRegion.right);
    const LONG bottom_border = min(roundRect.bottom - min(bw, half_height1), clipRegion.bottom);

    // circle related
    const LONG left_radius = max(roundRect.left + min(radius, half_width), clipRegion.left);
    const LONG top_radius = max(roundRect.top + min(radius, half_height), clipRegion.top);
    const LONG right_radius = min(roundRect.right - min(radius, half_width1), clipRegion.right);
    const LONG bottom_radius = min(roundRect.bottom - min(radius, half_height1), clipRegion.bottom);

    const LONG top_min_border_radius = min(top_border, top_radius);
    const LONG top_max_border_radius = max(top_border, top_radius);
    const LONG bottom_min_border_radius = min(bottom_border, bottom_radius);
    const LONG bottom_max_border_radius = max(bottom_border, bottom_radius);

    if (bg_col != bo_col) {
        // top
        for (LONG y = top; y < top_min_border_radius; y++) // border
            for (LONG x = left_radius; x < right_radius; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
        for (LONG y = top_border; y < top_radius; y++) // bg radius > border
            for (LONG x = left_radius; x < right_radius; x++)
                Op(pixels[y * canvasSize.cx + x], bg_col);
        for (LONG y = top_radius; y < top_border; y++) // border strip if radius < border
            for (LONG x = left; x < right; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);

        // mid
        for (LONG y = top_max_border_radius; y < bottom_min_border_radius; y++) {
            for (LONG x = left; x < left_border; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
            for (LONG x = left_border; x < right_border; x++)
                Op(pixels[y * canvasSize.cx + x], bg_col);
            for (LONG x = right_border; x < right; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
        }

        // bottom
        for (LONG y = bottom_border; y < bottom_radius; y++) // border strip if radius < border
            for (LONG x = left; x < right; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
        for (LONG y = bottom_radius; y < bottom_border; y++) // bg radius > border
            for (LONG x = left_radius; x < right_radius; x++)
                Op(pixels[y * canvasSize.cx + x], bg_col);
        for (LONG y = bottom_max_border_radius; y < bottom; y++) // border
            for (LONG x = left_radius; x < right_radius; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);

    } else {
        for (LONG y = top; y < top_radius; y++)
            for (LONG x = left_radius; x < right_radius; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);

        for (LONG y = top_radius; y < bottom_radius; y++)
            for (LONG x = left; x < right; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);

        for (LONG y = bottom_radius; y < bottom; y++)
            for (LONG x = left_radius; x < right_radius; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
    }

    // corners
    auto makeDist = [](LONG x, LONG y, LONG r) {
        float fx = float(r - x), fy = float(r - y);
        return r - sqrtf(fx * fx + fy * fy);
    };

    auto CompositeRadius = [&](DWORD& back, float dist) {
        dist += 1;
        float t = clamp(dist - bw, 0.f, 1.f);
        float a = clamp(dist, 0.f, 1.f);

        if (a > 0.f) {
            DWORD col = lerpColor(bo_col, bg_col, BYTE(t * 255));
            if constexpr (Op == compositeOverwrite) {
                back = lerpColor(back, col, BYTE(a * 255));
            } else {
                col = setAlpha(col, BYTE((col >> 24) * a));
                Op(back, col);
            }
        }
    };

    for (LONG y = top; y < top_radius; y++) { // top left
        for (LONG x = left; x < left_radius; x++) {
            float dist = makeDist(x - roundRect.left, y - roundRect.top, radius);
            CompositeRadius(pixels[y * canvasSize.cx + x], dist);
        }
        for (LONG x = right_radius; x < right; x++) {
            float dist = makeDist(width - x - 1 + roundRect.left, y - roundRect.top, radius);
            CompositeRadius(pixels[y * canvasSize.cx + x], dist);
        }
    }

    for (LONG y = bottom_radius; y < bottom; y++) { // bottom left
        for (LONG x = left; x < left_radius; x++) {
            float dist = makeDist(x - roundRect.left, height - y - 1 + roundRect.top, radius);
            CompositeRadius(pixels[y * canvasSize.cx + x], dist);
        }
        for (LONG x = right_radius; x < right; x++) {
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

    assert(false);
    // drawBorderedRectInternal<compositeAlphaInternal>(canvasSize, clipRect, pixels, roundRect, radius, bw, bg_col, bo_col);
}

void drawBorderedRectOverwrite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col)
{
    DWORD* pixels {};
    SIZE canvasSize;
    RECT clipRect;
    if (!validateCommon(hdc, roundRect, pixels, canvasSize, clipRect))
        return;

    drawBorderedRectInternal<compositeOverwrite>(canvasSize, clipRect, pixels, roundRect, radius, bw, bg_col, bo_col);
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

void drawGrayscaleMask(HDC hdc, const ImageBufferRLE img, const POINT pos, const RECT* customRect, DWORD color)
{
    if (img.data.empty())
        return;

    SIZE canvasSize;
    RECT clipRect;
    DWORD* canvasPixels;
    RECT bitmapRect = { pos.x, pos.y, pos.x + img.w, pos.y + img.h };

    if (!validateCommon(hdc, bitmapRect, canvasPixels, canvasSize, clipRect))
        return;

    if (customRect && !IntersectRect(&bitmapRect, &bitmapRect, customRect))
        return;

    int readIndex = 0;
    BYTE readRemain = img.data[0].first;
    BYTE value = img.data[0].second;

    for (int y = clipRect.top; y < clipRect.bottom; y++) {
        int hdcY = y * canvasSize.cx;
        // int stencilY = (y - pos.y) * img.w - pos.x;

        for (int x = clipRect.left; x < clipRect.right; x++) {
            compositeAlphaWithAlpha(canvasPixels[hdcY + x], color, 255 - value);

            readRemain--;
            if (readRemain == 0) {
                readIndex++;
                if (readIndex == img.data.size())
                    goto exit_loop;
                readRemain = img.data[readIndex].first;
                value = img.data[readIndex].second;
            }
        }
    }
exit_loop:;
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

    assert(false);
    //     drawBorderedRectInternal<replaceRGB>(bitmapSize, { 0, 0, bitmapSize.cx, bitmapSize.cy },
    //         pixels, roundRect, radius, 0, 0xFF000000 | col1, 0xFF000000 | col1);
}
