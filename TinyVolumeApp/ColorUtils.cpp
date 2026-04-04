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
        float cmax = std::max(c.r, std::max(c.g, c.b)) / 255.f;
        float cmin = std::min(c.r, std::min(c.g, c.b)) / 255.f;
        float sat = (cmax - cmin) / (1 - abs(2 * (cmax + cmin) / 2 - 1));
        return sat;
    };

    auto c = groups[0];
    float sat = calculateSaturation(c);
    float grayness = 1 - sat;
    c.r = lerp(c.r, 128, grayness * 0.3f);
    c.g = lerp(c.g, 128, grayness * 0.3f);
    c.b = lerp(c.b, 128, grayness * 0.3f);

    outColor = c.toDWORD();

    if (sat >= 0.5) {
        outColor2 = outColor;

    } else { // look for saturated secondary
        for (int i = 0; i < groups.size(); ++i) {
            auto& g = groups[i];
            if (calculateSaturation(g) > 0.5) {
                outColor2 = g.toDWORD();
                return true;
            }
        }
        outColor2 = groups[(groups.size() - 1) / 2].toDWORD();
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
