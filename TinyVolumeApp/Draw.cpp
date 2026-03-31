#define NOMINMAX

#include "Draw.h"

#include "Common.h"
#include "Utils.h"

#include <algorithm>
#include <math.h>

namespace {
inline void CompositeAlpha(DWORD& back, DWORD front)
{
    ARGB_SPLIT(BYTE, f, front);
    ARGB_SPLIT(BYTE, b, back);

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

inline void CompositeAlphaWithAlpha(DWORD& back, DWORD front, BYTE alpha)
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

inline void ReplaceRGB(DWORD& back, DWORD front)
{
    ARGB_SPLIT(BYTE, b, back);
    ARGB_SPLIT(BYTE, f, front);
    LERP_BYTE_COLOR(r, b, f, fa);
    back = (back & 0xFF000000) | (ARGB(ra, rr, rg, rb) & 0x00FFFFFF);
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
    DWORD* pixels, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col)
{
    int cl = (int)clipRegion.left, ct = (int)clipRegion.top, cr = (int)clipRegion.right, cb = (int)clipRegion.bottom;

    const int rcl = roundRect.left;
    const int rcr = roundRect.right;
    const int rct = roundRect.top;
    const int rcb = roundRect.bottom;
    const int rcw = roundRect.right - roundRect.left;
    const int rch = roundRect.bottom - roundRect.top;

    const int minrx = std::min(radius, std::max(rcw / 2, 0));
    const int minrx_right = std::min(radius, std::max((rcw + 1) / 2, 0));

    const int minry = std::min(radius, std::max(rch / 2, 0));
    const int minry_bottom = std::min(radius, std::max((rch + 1) / 2, 0));

    const int bwx = std::min(bw, minrx);
    const int bwy = std::min(bw, minry);
    const int bwy_bottom = std::min(bw, minry_bottom);

    const int rcr_minrx_start = std::max(rcr - minrx, cl);
    const int rcr_minrx_end = std::min(rcr - minrx_right, cr);

    const int rcl_start = std::max(rcl, cl);
    const int rct_start = std::max(rct, ct);

    const int rct_bwy = rct + bwy;
    const int rct_bwy_start = std::max(rct_bwy, ct);
    const int rct_bwy_end = std::min(rct_bwy, cb);

    const int rcr_bwx = rcr - bwx;
    const int rcr_bw_start = std::max(rcr_bwx, cl);
    const int rcr_bw_end = std::min(rcr_bwx, cr);

    const int rcb_bwy = rcb - bwy_bottom;
    const int rcb_bw_start = std::max(rcb_bwy, ct);
    const int rcb_bw_end = std::min(rcb_bwy, cb);

    const int rcb_minry = rcb - minry_bottom;
    const int rcb_minry_start = std::max(rcb_minry, ct);
    const int rcb_minry_end = std::min(rcb_minry, cb);

    const int rct_minry = rct + minry;
    const int rct_minry_start = std::max(rct_minry, ct);
    const int rct_minry_end = std::min(rct_minry, cb);

    const int rcl_bwx = rcl + bwx;
    const int rcl_bwx_start = std::max(rcl_bwx, cl);
    const int rcl_bwx_end = std::min(rcl_bwx, cr);

    const int rcl_minrx_start = std::max(rcl + minrx, cl);
    const int rcl_minrx_end = std::min(rcl + minrx_right, cr);

    const int rcb_end = std::min(rcb, cb);
    const int rcr_end = std::min(rcr, cr);

    if (rcl_minrx_start < rcr_minrx_end) {
        for (int y = rct_bwy_start; y < rct_minry_end; y++) // top section
            for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                Op(pixels[y * canvasSize.cx + x], bg_col);

        for (int y = rcb_minry_start; y < rcb_bw_end; y++) // bottom section
            for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                Op(pixels[y * canvasSize.cx + x], bg_col);

        for (int y = rct_start; y < rct_bwy_end; y++) // top border
            for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);

        for (int y = rcb_bw_start; y < rcb_end; y++) // bottom border
            for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
    }

    if (rcl_bwx_start < rcr_bw_end) {
        for (int y = rct_minry_start; y < rcb_minry_end; y++) // mid section
            for (int x = rcl_bwx_start; x < rcr_bw_end; x++)
                Op(pixels[y * canvasSize.cx + x], bg_col);
    }

    if (rcl_start < rcl_bwx_end) {
        for (int y = rct_minry_start; y < rcb_minry_end; y++) // left border
            for (int x = rcl_start; x < rcl_bwx_end; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
    }

    if (rcr_bw_start < rcr_end) {
        for (int y = rct_minry_start; y < rcb_minry_end; y++) // right border
            for (int x = rcr_bw_start; x < rcr_end; x++)
                Op(pixels[y * canvasSize.cx + x], bo_col);
    }

    // corners
    auto makeDist = [](int x, int y, int r) {
        float fx = float(r - x), fy = float(r - y);
        return r - sqrtf(fx * fx + fy * fy);
    };

    auto CompositeRadius = [&](DWORD& bkg, float dist) {
        dist += 1;
        float t = std::clamp(dist - bw, 0.f, 1.f);

        ARGB_SPLIT(float, a, bo_col);
        ARGB_SPLIT(float, b, bg_col);
        float a = (aa + t * (ba - aa)) * std::clamp(dist, 0.f, 1.f);
        float r = (ar + t * (br - ar));
        float g = (ag + t * (bg - ag));
        float b = (ab + t * (bb - ab));

        Op(bkg, ARGB(a, r, g, b));
    };

    if (rcl_start < rcl_minrx_end) {
        for (int y = rct_start; y < rct_minry_end; y++) // top left
            for (int x = rcl_start; x < rcl_minrx_end; x++) {
                float dist = makeDist(x - rcl, y - rct, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }

        for (int y = rcb_minry_start; y < rcb_end; y++) // bottom left
            for (int x = rcl_start; x < rcl_minrx_end; x++) {
                float dist = makeDist(x - rcl, rch - y - 1 + rct, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }
    }

    if (rcr_minrx_start < rcr_end) {
        for (int y = rct_start; y < rct_minry_end; y++) // top right
            for (int x = rcr_minrx_start; x < rcr_end; x++) {
                float dist = makeDist(rcw - x - 1 + rcl, y - rct, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }

        for (int y = rcb_minry_start; y < rcb_end; y++) // bottom right
            for (int x = rcr_minrx_start; x < rcr_end; x++) {
                float dist = makeDist(rcw - x - 1 + rcl, rch - y - 1 + rct, radius);
                CompositeRadius(pixels[y * canvasSize.cx + x], dist);
            }
    }
}
}

void drawBorderedRectAlphaComposite(HDC hdc, const RECT roundRect, int radius, int bw, DWORD bg_col, DWORD bo_col)
{
    DWORD* pixels {};
    SIZE canvasSize;
    RECT clipRegion;
    if (!validateCommon(hdc, roundRect, pixels, canvasSize, clipRegion))
        return;

    drawBorderedRectInternal<CompositeAlpha>(canvasSize, clipRegion, pixels, roundRect, radius, bw, bg_col, bo_col);
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
    RECT drawRect;
    DWORD* canvasPixels;
    if (!validateCommon(hdc, bitmapRect, canvasPixels, canvasSize, drawRect))
        return;

    if (customRect) {
        if (!IntersectRect(&bitmapRect, &bitmapRect, customRect))
            return;
    }

    int w = bitmapRect.right - bitmapRect.left;

    for (int y = bitmapRect.top; y < bitmapRect.bottom; y++) {
        int hdcY = y * canvasSize.cx;
        int stencilY = (y - pos.y) * bitmapSize.cx - pos.x;

        for (int x = bitmapRect.left; x < bitmapRect.right; x++) {
            DWORD pixelColor = bitmapPixels[stencilY + x];
            CompositeAlphaWithAlpha(canvasPixels[hdcY + x], pixelColor, alpha);
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

    drawBorderedRectInternal<ReplaceRGB>(bitmapSize, { 0, 0, bitmapSize.cx, bitmapSize.cy },
        pixels, roundRect, radius, 0, 0xFF000000 | col1, 0xFF000000 | col1);
}
