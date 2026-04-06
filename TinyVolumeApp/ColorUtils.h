#pragma once

typedef unsigned long DWORD;
typedef unsigned char BYTE;

#define ARGB(a, r, g, b) ((DWORD(a) << 24) | (DWORD(r) << 16) | (DWORD(g) << 8) | DWORD(b))

#define ARGB_SPLIT(T, name, dval)       \
    T name##a = T((dval >> 24) & 0xFF), \
      name##r = T((dval >> 16) & 0xFF), \
      name##g = T((dval >> 8) & 0xFF),  \
      name##b = T((dval) & 0xFF);

__forceinline DWORD lerpColor(DWORD a, DWORD b, BYTE alpha)
{
    ARGB_SPLIT(BYTE, a, a);
    ARGB_SPLIT(BYTE, b, b);

    BYTE ra = (aa + (ba - aa) * alpha / 255),
         rr = (ar + (br - ar) * alpha / 255),
         rg = (ag + (bg - ag) * alpha / 255),
         rb = (ab + (bb - ab) * alpha / 255);

    return ARGB(ra, rr, rg, rb);
}

__forceinline DWORD compositeAlpha(DWORD back, DWORD front)
{
    ARGB_SPLIT(BYTE, f, front);
    ARGB_SPLIT(BYTE, b, back);

    if (fa == 255)
        return front;

    if (fa == 0)
        return back;

    BYTE inv_a = 255 - fa;
    return ARGB(fa + (ba * inv_a) / 255,
        (fr * fa + br * inv_a) / 255,
        (fg * fa + bg * inv_a) / 255,
        (fb * fa + bb * inv_a) / 255);
}

class ColorUtils {
public:
    static bool calculatePriorityColor(DWORD* pixels, int width, int height, DWORD& outColor, DWORD& outColor2);
    static bool calculateAverage(DWORD* pixels, int width, int height, DWORD& outColor);
};
