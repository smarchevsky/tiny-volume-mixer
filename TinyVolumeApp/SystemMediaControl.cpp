#include "SystemMediaControl.h"

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

#pragma comment(lib, "windowsapp")

using namespace winrt::Windows::Media::Control;

void setActionInternal(SystemMediaControl::Action action)
{
}

void SystemMediaControl::setAction(Action action)
{
    std::thread worker([](Action action) {
        auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        auto session = manager.GetCurrentSession();
        if (session) {
            switch (action) {
            case Action::Pause: {
                session.TryPauseAsync();
            } break;
            case Action::Play: {
                session.TryPlayAsync();
            } break;
            case Action::Stop: {
                session.TryStopAsync();
            } break;
            }
        }
    },
        action);
    worker.join();
}

void RunMediaCheck()
{
    winrt::init_apartment();

    try {
        auto manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        auto sessions = manager.GetSessions();
        wprintf(L"NUM SESSIONS: %d\n", sessions.Size());
        for (auto const& session : sessions) {
            auto props = session.TryGetMediaPropertiesAsync().get();
            wprintf(L"- App: %s\n", session.SourceAppUserModelId().c_str());
            wprintf(L"  Title: %s\n", props.Title().c_str());
            // wprintf(L"  PlaybackRate: %f\n", session.GetPlaybackInfo().PlaybackRate().GetDouble());
        }
        printf("\n\n\n\n");
    } catch (winrt::hresult_error const& ex) {
        wprintf(L"Error: %s\n", ex.message().c_str());
    }
}

wchar_t SystemMediaControl::getPlaybackInfo()
{
    std::thread worker(RunMediaCheck);
    worker.join();
    return L'\0';
}
