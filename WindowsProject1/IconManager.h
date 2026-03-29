#pragma once
#include "Common.h"

#include <string>
#include <unordered_map>

constexpr DWORD defaultSliderColor = 0x00AAAAAA;

struct IconInfo {
    HBITMAP textBmp;
    HICON hLarge;
    DWORD ARGB;
};

class IconManager {
    IconManager() = default;
    IconManager(const IconManager&) = delete;
    int _iconSize = 24;

    std::unordered_map<std::wstring, IconInfo> _cachedAppIcons;
    IconInfo _iiMasterSpeaker {}, _iiMasterHeadphones {}, _iiNoIconApp {};

public:
    void init(int iconSize);
    void uninit();

    IconInfo* getIconMasterVol() { return &_iiMasterSpeaker; }
    IconInfo* tryRetrieveIcon(WCHAR* iconPath, PID pid);

    static IconManager& get()
    {
        static IconManager instance;
        return instance;
    }
};
