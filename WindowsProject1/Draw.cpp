#include "Draw.h"

#include "Common.h"

#include <algorithm>
#include <math.h>

namespace {
inline void CompositeAlpha(DWORD& back, float fa, float fr, float fg, float fb)
{
    ARGBf(b, back);

    if (fa == 255) {
        back = ARGB(fa, fr, fg, fb);
        return;
    }
    if (fa == 0)
        return;

    float inv_af = 255.f - fa;
    back = ARGB(
        fa + (ba * inv_af) / 255.f,
        (fr * fa + br * inv_af) / 255.f,
        (fg * fa + bg * inv_af) / 255.f,
        (fb * fa + bb * inv_af) / 255.f);
}

inline void CompositeAlpha(DWORD& back, DWORD front)
{
    ARGBf(f, front);
    CompositeAlpha(back, fa, fr, fg, fb);
}
}

void drawBorderedRect(HDC hdc, const RECT rc, int radius, int bw, DWORD bg_col, DWORD bo_col)
{
    DWORD* pixels;
    int canvasWidth, canvasHeight;
    getBitmapData(getBitmapFromHDC(hdc), canvasWidth, canvasHeight, pixels);

    if (!pixels)
        return;

    RECT canvasRect { 0, 0, canvasWidth, canvasHeight };

    RECT rcClip;
    int regionType = GetClipBox(hdc, &rcClip);
    if (regionType != ERROR && regionType != NULLREGION) {
        canvasRect.left = std::max(canvasRect.left, rcClip.left);
        canvasRect.top = std::max(canvasRect.top, rcClip.top);
        canvasRect.right = std::min(canvasRect.right, rcClip.right);
        canvasRect.bottom = std::max(canvasRect.bottom, rcClip.bottom);
    }

    RECT dest;
    if (!IntersectRect(&dest, &canvasRect, &rc))
        return;

    // printf("rcClip %d, %d, %d, %d\n", rcClip.left, rcClip.top, rcClip.right, rcClip.bottom);

    int cl = (int)canvasRect.left, ct = (int)canvasRect.top, cr = (int)canvasRect.right, cb = (int)canvasRect.bottom;

    const int rcl = rc.left;
    const int rcr = rc.right;
    const int rct = rc.top;
    const int rcb = rc.bottom;
    const int rcw = rc.right - rc.left;
    const int rch = rc.bottom - rc.top;

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
                    CompositeAlpha(pixels[y * canvasWidth + x], bg_col);

            for (int y = rcb_minry_start; y < rcb_bw_end; y++) // bottom section
                for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                    CompositeAlpha(pixels[y * canvasWidth + x], bg_col);
        }

        if (bo_col & 0xFF000000) {
            for (int y = rct_start; y < rct_bwy_end; y++) // top border
                for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                    CompositeAlpha(pixels[y * canvasWidth + x], bo_col);

            for (int y = rcb_bw_start; y < rcb_end; y++) // bottom border
                for (int x = rcl_minrx_start; x < rcr_minrx_end; x++)
                    CompositeAlpha(pixels[y * canvasWidth + x], bo_col);
        }
    }

    if (bg_col & 0xFF000000) {
        if (rcl_bwx_start < rcr_bw_end) {
            for (int y = rct_minry_start; y < rcb_minry_end; y++) // mid section
                for (int x = rcl_bwx_start; x < rcr_bw_end; x++)
                    CompositeAlpha(pixels[y * canvasWidth + x], bg_col);
        }
    }

    if (bo_col & 0xFF000000) {
        if (rcl_start < rcl_bwx_end) {
            for (int y = rct_minry_start; y < rcb_minry_end; y++) // left border
                for (int x = rcl_start; x < rcl_bwx_end; x++)
                    CompositeAlpha(pixels[y * canvasWidth + x], bo_col);
        }

        if (rcr_bw_start < rcr_end) {
            for (int y = rct_minry_start; y < rcb_minry_end; y++) // right border
                for (int x = rcr_bw_start; x < rcr_end; x++)
                    CompositeAlpha(pixels[y * canvasWidth + x], bo_col);
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

        ARGBf(a, bo_col);
        ARGBf(b, bg_col);
        float a = (aa + t * (ba - aa)) * std::clamp(dist, 0.f, 1.f);
        float r = (ar + t * (br - ar));
        float g = (ag + t * (bg - ag));
        float b = (ab + t * (bb - ab));

        CompositeAlpha(bkg, a, r, g, b);
        return bkg;
    };

    if (rcl_start < rcl_minrx_end) {
        for (int y = rct_start; y < rct_minry_end; y++) // top left
            for (int x = rcl_start; x < rcl_minrx_end; x++) {
                float dist = makeDist(x - rcl, y - rct, radius);
                CompositeRadius(pixels[y * canvasWidth + x], dist);
            }

        for (int y = rcb_minry_start; y < rcb_end; y++) // bottom left
            for (int x = rcl_start; x < rcl_minrx_end; x++) {
                float dist = makeDist(x - rcl, rch - y - 1 + rct, radius);
                CompositeRadius(pixels[y * canvasWidth + x], dist);
            }
    }

    if (rcr_minrx_start < rcr_end) {
        for (int y = rct_start; y < rct_minry_end; y++) // top right
            for (int x = rcr_minrx_start; x < rcr_end; x++) {
                float dist = makeDist(rcw - x - 1 + rcl, y - rct, radius);
                CompositeRadius(pixels[y * canvasWidth + x], dist);
            }

        for (int y = rcb_minry_start; y < rcb_end; y++) // bottom right
            for (int x = rcr_minrx_start; x < rcr_end; x++) {
                float dist = makeDist(rcw - x - 1 + rcl, rch - y - 1 + rct, radius);
                CompositeRadius(pixels[y * canvasWidth + x], dist);
            }
    }
}