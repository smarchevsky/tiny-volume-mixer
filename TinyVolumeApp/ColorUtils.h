#pragma once

typedef unsigned long DWORD;
typedef unsigned char BYTE;

#define ARGB(a, r, g, b) ((DWORD(a) << 24) | (DWORD(r) << 16) | (DWORD(g) << 8) | DWORD(b))

#define ARGB_SPLIT(T, name, dval)       \
    T name##a = T((dval >> 24) & 0xFF), \
      name##r = T((dval >> 16) & 0xFF), \
      name##g = T((dval >> 8) & 0xFF),  \
      name##b = T((dval) & 0xFF);

#define LERP_BYTE_COLOR(result, col0, col1, x)                    \
    BYTE result##a = (col0##a + (col1##a - col0##a) * (x) / 255), \
         result##r = (col0##r + (col1##r - col0##r) * (x) / 255), \
         result##g = (col0##g + (col1##g - col0##g) * (x) / 255), \
         result##b = (col0##b + (col1##b - col0##b) * (x) / 255);

class ColorUtils {
public:
    static bool calculatePriorityColor(DWORD* pixels, int width, int height, DWORD& outColor);
    static bool calculateAverage(DWORD* pixels, int width, int height, DWORD& outColor);
};
