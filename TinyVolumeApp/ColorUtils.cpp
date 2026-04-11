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

        result.r = cr / 255, result.g = cg / 255, result.b = cb / 255;

        const float minDistances[] = {
            getColorDistSq(result, { 0.0f, 0.6f, 1.0f }) + 0.01f,
            getColorDistSq(result, { 1.0f, 0.0f, 0.5f }) + 0.01f,
            getColorDistSq(result, { 0.4f, 1.0f, 0.0f }) + 0.01f,
        };

        float minDist = INFINITY;
        for (auto m : minDistances)
            minDist = std::min(m, minDist);

        result.w = weight / minDist;

        return result;
    }

    DWORD toDWORD() const { return ARGB(0,
        std::clamp<float>(r, 0, 1) * 255,
        std::clamp<float>(g, 0, 1) * 255,
        std::clamp<float>(b, 0, 1) * 255); }

    ColorWeight& operator+=(const ColorWeight& rhs)
    {
        float sumWeightRecip = 1.f / (w + rhs.w);
        r = (r * w + rhs.r * rhs.w) * sumWeightRecip;
        g = (g * w + rhs.g * rhs.w) * sumWeightRecip;
        b = (b * w + rhs.b * rhs.w) * sumWeightRecip;
        w += rhs.w;
        return *this;
    }

    inline static ColorWeight lerp(ColorWeight c1, const ColorWeight& c2, float x)
    {
        c1.r = ::lerp(c1.r, c2.r, x), c1.g = ::lerp(c1.g, c2.g, x), c1.b = ::lerp(c1.b, c2.b, x);
        return c1;
    }

    inline static float getColorDistSq(const ColorWeight& c1, const ColorWeight& c2)
    {
        float dr = c2.r - c1.r, dg = c2.g - c1.g, db = c2.b - c1.b;
        return (dr * dr + dg * dg + db * db);
    };
};
}

bool ColorUtils::calculatePriorityColor(DWORD* pixels, int width, int height, DWORD& outColor, DWORD& outColor2)
{
    std::unordered_map<DWORD, int> colorGroups;
    const int pixelCount = width * height;
    for (int i = 0; i < pixelCount; i += 2) {
        DWORD pixel = pixels[i];
        ARGB_SPLIT(BYTE, , pixel)
        if (a > 127) {
            colorGroups[pixel & 0x00F0F0F0]++;
        }
    }

    if (colorGroups.empty())
        return false;

    std::vector<ColorWeight> groups;
    for (auto& [dwColor, num] : colorGroups)
        if (num >= 10)
            groups.push_back(ColorWeight::fromHexAndWeight(dwColor, (float)num));

    if (groups.size() == 0)
        return false;

    std::sort(groups.begin(), groups.end(), [](const ColorWeight& a, const ColorWeight& b) { return a.w > b.w; });

    auto calculateSaturation = [](ColorWeight& c) {
        float cmax = std::max(c.r, std::max(c.g, c.b));
        float cmin = std::min(c.r, std::min(c.g, c.b));
        float sat = (cmax - cmin) / (1 - abs(2 * (cmax + cmin) / 2 - 1));
        return sat;
    };

    auto finalColor = groups[0];
    float sat = calculateSaturation(finalColor);

    ColorWeight greyedDesaturatedColor = finalColor;
    greyedDesaturatedColor = ColorWeight::lerp(greyedDesaturatedColor, { 0.5f, 0.5f, 0.5f }, (1 - sat) * 0.3f);

    outColor = greyedDesaturatedColor.toDWORD();
    outColor2 = finalColor.toDWORD();

    if (sat < 0.5) {
        for (int i = 0; i < groups.size(); ++i) {
            auto& g = groups[i];
            if (calculateSaturation(g) > 0.5) {
                outColor2 = g.toDWORD();
                return true;
            }
        }
        outColor2 = groups[(groups.size() - 1) / 4].toDWORD();
    }

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
