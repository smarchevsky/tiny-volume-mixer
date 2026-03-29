#define NOMINMAX

#include "Draw.h"

#include "Common.h"
#include "Utils.h"

#include <algorithm>
#include <math.h>

#define ARGBsplit(T, name, dval) T name##a = T((dval >> 24) & 0xFF), name##r = T((dval >> 16) & 0xFF), name##g = T((dval >> 8) & 0xFF), name##b = T((dval) & 0xFF);

namespace {
inline void CompositeAlpha(DWORD& back, DWORD front)
{
    BYTE fa = (front >> 24) & 0xFF;
    if (fa == 255) {
        back = front;
        return;
    }
    if (fa == 0) {
        return;
    }

    BYTE fr = (front >> 16) & 0xFF;
    BYTE fg = (front >> 8) & 0xFF;
    BYTE fb = front & 0xFF;

    BYTE ba = (back >> 24) & 0xFF;
    BYTE br = (back >> 16) & 0xFF;
    BYTE bg = (back >> 8) & 0xFF;
    BYTE bb = back & 0xFF;

    BYTE inv_a = 255 - fa;
    back = ARGB(
        fa + (ba * inv_a) / 255,
        (fr * fa + br * inv_a) / 255,
        (fg * fa + bg * inv_a) / 255,
        (fb * fa + bb * inv_a) / 255);
}

inline void ReplaceRGB(DWORD& back, DWORD front)
{
    back = (back & 0xFF000000) | (front & 0x00FFFFFF);
}

bool validateCommon(HDC hdc, RECT roundRect, DWORD*& pixels, SIZE& canvasSize, RECT& drawRect)
{
    getBitmapData(getBitmapFromHDC(hdc), canvasSize, pixels);
    if (!pixels)
        return false;

    drawRect = { 0, 0, canvasSize.cx, canvasSize.cy };

    RECT rcClip;
    int regionType = GetClipBox(hdc, &rcClip);
    if (regionType != ERROR && regionType != NULLREGION) {
        drawRect.left = std::max(drawRect.left, rcClip.left);
        drawRect.top = std::max(drawRect.top, rcClip.top);
        drawRect.right = std::min(drawRect.right, rcClip.right);
        drawRect.bottom = std::max(drawRect.bottom, rcClip.bottom);
        // printf("rcClip %d, %d, %d, %d\n", rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);
    }

    if (!IntersectRect(&drawRect, &drawRect, &roundRect))
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
        if (bg_col & 0xFF000000) {
            for (int y = rct_bwy_start; y < rct_minry_end; y++) // top section
                for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bg_col);

            for (int y = rcb_minry_start; y < rcb_bw_end; y++) // bottom section
                for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bg_col);
        }

        if (bo_col & 0xFF000000) {
            for (int y = rct_start; y < rct_bwy_end; y++) // top border
                for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bo_col);

            for (int y = rcb_bw_start; y < rcb_end; y++) // bottom border
                for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bo_col);
        }
    }

    if (bg_col & 0xFF000000) {
        if (rcl_bwx_start < rcr_bw_end) {
            for (int y = rct_minry_start; y < rcb_minry_end; y++) // mid section
                for (int x = rcl_bwx_start; x < rcr_bw_end; x++)
                    Op(pixels[y * canvasSize.cx + x], bg_col);
        }
    }

    if (bo_col & 0xFF000000) {
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
    }

    // corners
    auto makeDist = [](int x, int y, int r) {
        float fx = float(r - x), fy = float(r - y);
        return r - sqrtf(fx * fx + fy * fy);
    };

    auto CompositeRadius = [&](DWORD& bkg, float dist) {
        dist += 1;
        float t = std::clamp(dist - bw, 0.f, 1.f);

        ARGBsplit(float, a, bo_col);
        ARGBsplit(float, b, bg_col);
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

#define LERP_BYTE(result, a, b)                             \
    BYTE result##a = (aa + (ba - aa) * stencilAlpha / 255); \
    BYTE result##r = (ar + (br - ar) * stencilAlpha / 255); \
    BYTE result##g = (ag + (bg - ag) * stencilAlpha / 255); \
    BYTE result##b = (ab + (bb - ab) * stencilAlpha / 255);

void drawBitmapAlphaComposite(HDC hdc, HBITMAP bmp, const POINT pos, int horizontalShift)
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

    int h = drawRect.bottom - drawRect.top;
    int w = drawRect.right - drawRect.left;

    for (int y = 0; y < h; y++) {
        int hdcY = (y + pos.y) * canvasSize.cx + pos.x;
        int stencilY = y * bitmapSize.cx;
        for (int x = 0; x < w; x++) {
            DWORD pixelColor = bitmapPixels[stencilY + x];
            CompositeAlpha(canvasPixels[hdcY + (horizontalShift + x) % w], pixelColor);
        }
    }
}
