#pragma once

#include "Common.h"

#include <AudioSessionTypes.h>
#include <combaseapi.h>
#include <utility> // std::forward
#include <vector>

struct AudioUpdateInfo {

    union {
        struct {
            WPARAM _wp;
            LPARAM _lp;
        };
        struct {
            PID _pid;
            float _vol;
            bool _isMuted;
            VolumeType _type;
        };
    };
    AudioUpdateInfo(VolumeType type, PID pid, float vol, bool isMuted) { _pid = pid, _vol = vol, _isMuted = isMuted, _type = type; }
    AudioUpdateInfo(WPARAM wp, LPARAM lp) { _wp = wp, _lp = lp; }
};

struct ActivationChangedInfo {
    PID pid;
    bool active;
};

static_assert(sizeof(WPARAM) == 8);
static_assert(sizeof(LPARAM) == 8);
static_assert(sizeof(AudioUpdateInfo) == 16);
static_assert(sizeof(ActivationChangedInfo) <= sizeof(WPARAM));

struct AudioSessionInitInfo {
    AudioUpdateInfo updateInfo;
    AudioSessionState audioSessionState {};
    WCHAR *displayName {}, *iconPath {};

    template <typename... Args>
    AudioSessionInitInfo(Args&&... args)
        : updateInfo(std::forward<Args>(args)...)
    {
    }
    ~AudioSessionInitInfo()
    {
        CoTaskMemFree(iconPath);
        CoTaskMemFree(displayName);
    }
};

class CoinitializeWrapper {
public:
    CoinitializeWrapper();
    ~CoinitializeWrapper();
};

struct WaveInfo {
    PID pid;
    float wave;
};

struct IMMDeviceEnumerator;
struct IMMDevice;
struct IAudioEndpointVolume; // master
struct IAudioEndpointVolumeCallback;
struct IAudioSessionManager2; // apps
struct IAudioSessionNotification;

class AudioUpdateListener {
    IMMDeviceEnumerator* _pMMDeviceEnumerator {};
    IMMDevice* _pMMDevice {};
    IAudioEndpointVolume* _pEndpointVolume {}; // master
    IAudioEndpointVolumeCallback* _pEndpointVolumeCallback {}; // master
    IAudioSessionManager2* _pSessionManager2 {}; // apps
    IAudioSessionNotification* _pSessionNotification {}; // apps

public:
    AudioUpdateListener() = default;

    void init(HWND hWnd);
    void uninit();

    void cleanupExpiredSessions();
    void setVol(SelectInfo selectInfo, float vol);
    bool retieveWaveInfo(std::vector<WaveInfo>& waveInfo);
};