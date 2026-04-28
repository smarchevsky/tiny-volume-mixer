#pragma once

class SystemMediaControl {
public:
    enum Action {
        Pause,
        Play,
        Stop
    };

    static void setAction(Action action);
    static wchar_t getPlaybackInfo();
};
