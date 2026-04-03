#define NOMINMAX

#include "ColorUtils.h"
// #include "Timer.h"

#include <algorithm>
#include <unordered_map>

namespace {
float lerp(float a, float b, float x) { return a + (b - a) * x; }

struct ColorWeight {
    float r, g, b, w;

    static ColorWeight fromHexAndWeight(DWORD colorHex, float weight)
    {
        ColorWeight result;
        ARGB_SPLIT(float, c, colorHex);

        result.r = cr, result.g = cg, result.b = cb;

        const float minDistances[] = {
            getColorDistSq(result, { 0, 148, 255 }) + 1000,
            getColorDistSq(result, { 254, 0, 128 }) + 1000,
            getColorDistSq(result, { 100, 255, 0 }) + 1000,
        };

        float minDist = INFINITY;
        for (auto m : minDistances)
            minDist = std::min(m, minDist);

        result.w = weight / minDist;
        return result;
    }

    DWORD toDWORD() const { return ARGB(0,
        std::clamp(r, 0.f, 255.f),
        std::clamp(g, 0.f, 255.f),
        std::clamp(b, 0.f, 255.f)); }

    ColorWeight& operator+=(const ColorWeight& rhs)
    {
        float sumWeightRecip = 1.f / (w + rhs.w);
        r = (r * w + rhs.r * rhs.w) * sumWeightRecip;
        g = (g * w + rhs.g * rhs.w) * sumWeightRecip;
        b = (b * w + rhs.b * rhs.w) * sumWeightRecip;
        w += rhs.w;
        return *this;
    }

    inline static float getColorDistSq(const ColorWeight& c1, const ColorWeight& c2)
    {
        float dr = c2.r - c1.r, dg = c2.g - c1.g, db = c2.b - c1.b;
        return (dr * dr + dg * dg + db * db);
    };
};
}

bool ColorUtils::calculatePriorityColor(DWORD* pixels, int width, int height, DWORD& outColor)
{
    std::unordered_map<DWORD, int> colorGroups;
    const int pixelCount = width * height;
    for (int i = 0; i < pixelCount; i++) {
        DWORD pixel = pixels[i];
        ARGB_SPLIT(BYTE, , pixel)
        if (a > 127) {
            colorGroups[pixel & 0x00F0F0F0]++;
        }
    }

    if (colorGroups.empty())
        return false;

    std::vector<ColorWeight> g0;
    for (auto& [dwColor, num] : colorGroups)
        if (num >= 10)
            g0.push_back(ColorWeight::fromHexAndWeight(dwColor, (float)num));

    if (g0.size() == 0)
        return false;

    std::sort(g0.begin(), g0.end(), [](const ColorWeight& a, const ColorWeight& b) { return a.w > b.w; });

    int collected = 0;

    ColorWeight finalColor = g0[std::min((int)g0.size() - 1, 0)];
    for (int i = 1; i < g0.size(); i++) {
        float distSq = ColorWeight::getColorDistSq(g0[i], finalColor);
        if (distSq < powf(50.f, 2.f)) {
            finalColor += g0[i];
            collected++;
        }
    }

    auto makeDesaturatedLighter = [](ColorWeight& c) {
        float cmax = std::max(c.r, std::max(c.g, c.b)) / 255.f;
        float cmin = std::min(c.r, std::min(c.g, c.b)) / 255.f;
        float sat = (cmax - cmin) / (1 - abs(2 * (cmax + cmin) / 2 - 1));
        float grayness = 1 - sat;
        grayness *= 0.5f;
        c.r = lerp(c.r, 200, grayness);
        c.g = lerp(c.g, 200, grayness);
        c.b = lerp(c.b, 200, grayness);
    };

    // finalColor = g0[0];
    makeDesaturatedLighter(finalColor);
    outColor = finalColor.toDWORD();

    return true;
}

bool ColorUtils::calculateAverage(DWORD* pixels, int width, int height, DWORD& outColor)
{
    uint64_t totalR = 0, totalG = 0, totalB = 0;
    int opaquePixels = 0;

    const int pixelCount = width * height;
    for (int i = 0; i < pixelCount; i++) {
        DWORD pixel = pixels[i];
        ARGB_SPLIT(BYTE, , pixel);
        if (a > 127)
            totalR += r, totalG += g, totalB += b, opaquePixels++;
    }

    if (opaquePixels > 0)
        return ARGB(0, BYTE(totalR / opaquePixels), BYTE(totalG / opaquePixels), BYTE(totalB / opaquePixels));
    return false;
}
