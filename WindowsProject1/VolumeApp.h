#pragma once

#include "App.h"

class VolumeApp : public App {
    bool fMouseTracking;

public:
    void handlePreLoop(WNDPROC winProc);
    void shutDown(HWND hWnd);

    void handleMouseMove(POINT cursorScreenPos);
    void handleMouseLeave();
    void setWindowSemiTransparent(bool semiTransparent);
};
