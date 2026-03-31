#include "AudioUtils.h"

#include <endpointvolume.h> // IAudioEndpointVolume, IAudioEndpointVolumeCallback
#include <mmdeviceapi.h> // IMMDevice, IMMDeviceEnumerator

#include <audiopolicy.h>

#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#define CHECK_REF_COUNT(object)                      \
    do {                                             \
        object->AddRef();                            \
        ULONG refCount = object->Release();          \
        wprintf(L"REFERENCE COUNT: %u\n", refCount); \
    } while (0);

struct IAudioSessionControl;
struct IAudioSessionEvents;

// S_OK, S_FALSE
CoinitializeWrapper::CoinitializeWrapper() { (void)CoInitialize(NULL); }
CoinitializeWrapper::~CoinitializeWrapper() { CoUninitialize(); }

namespace {
struct ExpiredSession {
    IAudioSessionControl* pCtrl;
    IAudioSessionEvents* pEvents;
};

struct ActiveSession {
    IAudioSessionControl* pCtrl;
    PID pid;
    AudioSessionState state;
};

std::vector<ExpiredSession> g_expiredSessions;
std::vector<ActiveSession> g_trackedSessions;
std::mutex g_mutex;

class CVolumeNotification : public IAudioEndpointVolumeCallback {
    HWND _hWnd;
    LONG _cRef;

public:
    CVolumeNotification(HWND hWnd)
        : _hWnd(hWnd)
        , _cRef(1)
    {
    }

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&_cRef); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG ref = InterlockedDecrement(&_cRef);
        if (ref == 0)
            delete this;
        return ref;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == IID_IUnknown || riid == __uuidof(IAudioEndpointVolumeCallback)) {
            *ppv = static_cast<IAudioEndpointVolumeCallback*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // Called when volume/mute changes
    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override
    {
        if (!pNotify)
            return E_INVALIDARG;
        AudioUpdateInfo info(VolumeType::Master, (PID)0, pNotify->fMasterVolume, pNotify->bMuted);
        PostMessage(_hWnd, WM_REFRESH_VOL, info._wp, info._lp);
        return S_OK;
    }
};

class AudioSessionEvents : public IAudioSessionEvents {
    LONG _cRef;
    PID _pid;
    IAudioSessionControl* _pCtrl; // back-reference to owning session
    HWND _hWnd;

public:
    AudioSessionEvents(PID pid, IAudioSessionControl* pCtrl, HWND hWnd)
        : _cRef(1)
        , _pid(pid)
        , _pCtrl(pCtrl)
        , _hWnd(hWnd)
    {
    }

    ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&_cRef); }
    ULONG STDMETHODCALLTYPE Release()
    {
        ULONG r = InterlockedDecrement(&_cRef);
        if (!r) {
            wprintf(L"Delete this called [PID %u]\n", _pid);
            delete this;
        }
        return r;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == IID_IUnknown || riid == __uuidof(IAudioSessionEvents)) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(
        float newVol, BOOL newMute, LPCGUID) override
    {
        AudioUpdateInfo info(VolumeType::App, _pid, newVol, newMute);
        PostMessage(_hWnd, WM_REFRESH_VOL, info._wp, info._lp);
        // wprintf(L"[PID %u] Volume: %.2f | Muted: %s\n", _pid, newVol, newMute ? L"Yes" : L"No");
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(LPCWSTR, LPCGUID) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnIconPathChanged(LPCWSTR, LPCGUID) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(DWORD, float[], DWORD, LPCGUID) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(LPCGUID, LPCGUID) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnStateChanged(AudioSessionState newState) override
    { // expired when app naturally closed
        if (newState == AudioSessionStateExpired) {
            // wprintf(L"OnStateChanged, AudioSessionStateExpired [PID %u]\n", _pid);
            Cleanup();

        } else {
            PID changedStatePID = PID(-1);
            bool activeAny = false;
            for (auto& s : g_trackedSessions) {
                if (s.pCtrl == _pCtrl) {
                    s.state = newState;
                    changedStatePID = s.pid;
                }
                activeAny |= s.state == AudioSessionState::AudioSessionStateActive;
            }

            ActivationChangedInfo info;
            info.pid = changedStatePID;
            info.active = newState == AudioSessionState::AudioSessionStateActive;
            info.activeAny = activeAny;

            static_assert(sizeof(ActivationChangedInfo) <= sizeof(WPARAM));
            PostMessage(_hWnd, WM_APP_ACTIVATION_CHANGED, reinterpret_cast<WPARAM&>(info), 0);

            // wprintf(L"State changed: [PID %u], state: %s\n", _pid, newState == AudioSessionStateActive ? L"active" : L"inactive");
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnSessionDisconnected(AudioSessionDisconnectReason) override
    { // disconnected when app crashes
        wprintf(L"OnSessionDisconnected [PID %u]\n", _pid);
        Cleanup();
        return S_OK;
    }

    void Cleanup()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = std::find_if(g_trackedSessions.begin(), g_trackedSessions.end(),
            [&](const ActiveSession& s) { return s.pCtrl == _pCtrl; });

        if (it != g_trackedSessions.end()) {
            g_trackedSessions.erase(it);
            g_expiredSessions.push_back({ _pCtrl, this });
            AddRef();
        }
        AudioUpdateInfo info(VolumeType::App, _pid, 0, false);
        PostMessage(_hWnd, WM_APP_UNREGISTERED, info._wp, info._lp);
    }
};

std::wstring guid2str(REFGUID guid)
{
    WCHAR szGuid[40];
    if (StringFromGUID2(guid, szGuid, ARRAYSIZE(szGuid)) != 0)
        return szGuid;
    return {};
}

static void addSessionImpl(IAudioSessionControl* pCtrl, HWND hWnd, const wchar_t* dbgActionName)
{
    IAudioSessionControl2* pCtrl2 = nullptr;
    pCtrl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pCtrl2);

    wchar_t *displayName {}, *iconPath {};
    AudioSessionState audioSessionState {};
    PID pid = 0;
    if (pCtrl2) {
        pCtrl2->GetProcessId(&pid);
        pCtrl2->GetState(&audioSessionState);
        pCtrl2->GetDisplayName(&displayName);
        pCtrl2->GetIconPath(&iconPath);

        pCtrl2->Release();
    }

    ISimpleAudioVolume* pVol = NULL;
    if (SUCCEEDED(pCtrl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pVol))) {
        BOOL bMute;
        float vol;
        pVol->GetMasterVolume(&vol);
        pVol->GetMute(&bMute);

        auto audioSessionInitInfo = new AudioSessionInitInfo(VolumeType::App, pid, vol, bMute);
        audioSessionInitInfo->audioSessionState = audioSessionState;
        audioSessionInitInfo->displayName = displayName;
        audioSessionInitInfo->iconPath = iconPath;

        wprintf(L"%s [PID %u], displayName %s, iconPath %s, vol: %d\n", dbgActionName, pid,
            displayName, iconPath, int(vol * 100));

        PostMessage(hWnd, WM_APP_REGISTERED, (WPARAM)audioSessionInitInfo, 0);
        pVol->Release();
    }

    AudioSessionEvents* pEvents = new AudioSessionEvents(pid, pCtrl, hWnd);
    pCtrl->RegisterAudioSessionNotification(pEvents);
    pEvents->Release();

    std::lock_guard<std::mutex> lock(g_mutex);
    g_trackedSessions.push_back({ pCtrl, pid, audioSessionState });
}

class SessionNotification : public IAudioSessionNotification {
    LONG _cRef;
    HWND _hWnd;

public:
    SessionNotification(HWND hWnd)
        : _cRef(1)
        , _hWnd(hWnd)
    {
    }

    ULONG STDMETHODCALLTYPE AddRef() { return InterlockedIncrement(&_cRef); }
    ULONG STDMETHODCALLTYPE Release()
    {
        ULONG r = InterlockedDecrement(&_cRef);
        if (!r)
            delete this;
        return r;
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == IID_IUnknown || riid == __uuidof(IAudioSessionNotification)) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // Fired when ANY new audio session (new app) starts playing
    HRESULT STDMETHODCALLTYPE OnSessionCreated(IAudioSessionControl* pCtrl) override
    {
        pCtrl->AddRef();
        addSessionImpl(pCtrl, _hWnd, L"New session");
        return S_OK;
    }
};

void RegisterAllExistingSessions(IAudioSessionManager2* pMgr, HWND hWnd)
{
    IAudioSessionEnumerator* pEnum = nullptr;
    pMgr->GetSessionEnumerator(&pEnum);

    int count = 0;
    pEnum->GetCount(&count);

    for (int i = 0; i < count; i++) {
        IAudioSessionControl* pCtrl = nullptr;
        pEnum->GetSession(i, &pCtrl);
        addSessionImpl(pCtrl, hWnd, L"Registering");
    }

    pEnum->Release();
}
}

void AudioUpdateListener::init(HWND hWnd)
{
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), (void**)&_pMMDeviceEnumerator);
    _pMMDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &_pMMDevice);

    // master
    _pMMDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, (void**)&_pEndpointVolume);

    // extract application vol
    BOOL bMute;
    float vol;
    _pEndpointVolume->GetMasterVolumeLevelScalar(&vol);
    _pEndpointVolume->GetMute(&bMute);
    AudioUpdateInfo info(VolumeType::Master, (PID)0, vol, bMute);
    PostMessage(hWnd, WM_REFRESH_VOL, info._wp, info._lp);

    // create application event listener
    _pEndpointVolumeCallback = new CVolumeNotification(hWnd);
    _pEndpointVolume->RegisterControlChangeNotify(_pEndpointVolumeCallback);

    // apps
    _pMMDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&_pSessionManager2);
    RegisterAllExistingSessions(_pSessionManager2, hWnd);
    _pSessionNotification = new SessionNotification(hWnd);
    _pSessionManager2->RegisterSessionNotification(_pSessionNotification);
}

void AudioUpdateListener::uninit()
{
    // apps
    _pSessionManager2->UnregisterSessionNotification(_pSessionNotification);
    _pSessionNotification->Release();
    _pSessionManager2->Release();

    // master
    _pEndpointVolume->UnregisterControlChangeNotify(_pEndpointVolumeCallback);
    _pEndpointVolumeCallback->Release();
    _pEndpointVolume->Release();

    _pMMDevice->Release();
    _pMMDeviceEnumerator->Release();
}

void AudioUpdateListener::cleanupExpiredSessions()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& e : g_expiredSessions) {
        e.pCtrl->UnregisterAudioSessionNotification(e.pEvents);
        e.pEvents->Release(); // pair for AddRef above
        e.pCtrl->Release(); // pair for AddRef in OnSessionCreated
    }
    g_expiredSessions.clear();
    printf("Cleanup session, g_trackedSessions num %llu\n", g_trackedSessions.size());
}

void AudioUpdateListener::setVol(SelectInfo selectInfo, float vol)
{
    if (selectInfo._type == VolumeType::Master) {
        if (_pEndpointVolume) {
            _pEndpointVolume->SetMasterVolumeLevelScalar(vol, nullptr);
            // wprintf(L"Setting master vol: %d\n", (int)(vol * 100));
        }

    } else if (selectInfo._type == VolumeType::App) {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (auto& trackedSession : g_trackedSessions) {
            if (trackedSession.pid == selectInfo._pid) {
                ISimpleAudioVolume* pVolume = nullptr;
                trackedSession.pCtrl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&pVolume);
                if (pVolume) {
                    pVolume->SetMasterVolume(vol, nullptr);
                    pVolume->Release();
                    // wprintf(L"Setting app vol [PID %u], vol: %d\n", selectInfo._pid, (int)(vol * 100));
                }
                break;
            }
        }
    }
}