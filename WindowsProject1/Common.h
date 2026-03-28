#pragma once

#include <Windows.h>

#include <stdint.h>

#define ARGB(a, r, g, b) ((DWORD(a) << 24) | (DWORD(r) << 16) | (DWORD(g) << 8) | DWORD(b))

typedef DWORD PID;

enum {
    _____WM_APP = WM_APP,
    WM_REFRESH_VOL,
    WM_APP_REGISTERED,
    WM_APP_UNREGISTERED,
};

enum class VolumeType : uint8_t {
    Invalid,
    Master,
    App
};

struct SelectInfo {
    SelectInfo(VolumeType type, PID pid) { _type = type, _pid = pid; }
    SelectInfo() = default;
    bool operator==(const SelectInfo& rhs) const { return _type == rhs._type && _pid == rhs._pid; }
    bool operator!=(const SelectInfo& rhs) const { return !operator==(rhs); }
    operator bool() const { return _type != VolumeType::Invalid; }
    PID _pid;
    VolumeType _type;
};

class PixelBuffer_RG {
    BYTE* buf {}; // w, h, offset1, data0, data1
    static constexpr int sizeSize = sizeof(LONG) * 3;
    inline LONG getIntData(int index) const { return *((LONG*)buf + index); }
    inline LONG& getIntData(int index) { return *((LONG*)buf + index); }

public:
    SIZE getSize() const { return { getIntData(0), getIntData(1) }; }
    template <int ChannelIndex>
    const BYTE* data() const
    {
        static_assert(ChannelIndex == 0 || ChannelIndex == 1);
        if (!buf)
            return nullptr;
        if constexpr (ChannelIndex == 0)
            return buf + sizeSize;
        else
            return buf + getIntData(2);
    }

    template <int ChannelIndex>
    BYTE* data()    
    {
        return const_cast<BYTE*>(const_cast<const PixelBuffer_RG*>(this)->data<ChannelIndex>());
    }

    void allocateSize(int w, int h)
    {
        int singleChannelSize = w * h;
        buf = (BYTE*)malloc(sizeSize + singleChannelSize * 2);
        getIntData(0) = w, getIntData(1) = h;
        getIntData(2) = sizeSize + singleChannelSize;
    }

    void cleanup()
    {
        if (buf)
            free(buf);
        buf = nullptr;
    }
};

struct UIConfig {
    uint8_t frameBorderWidth = 3;
    uint8_t sliderSpacing = 8;
    uint8_t sliderCornerRadius = 8;
    uint8_t sliderWidthApp = 80;
    uint8_t sliderWidthMaster = 100;
    uint8_t sliderBorderWidth = frameBorderWidth;
    uint8_t frameCornerRadius = sliderCornerRadius + sliderSpacing + frameBorderWidth;
    uint8_t iconSize = 48;

    int getSliderOffsetL() const { return sliderSpacing / 2; }
    int getSliderOffsetR() const { return sliderSpacing - getSliderOffsetL(); }

    static UIConfig get()
    {
        UIConfig config;
        return config;
    }
};
