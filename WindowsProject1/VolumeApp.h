#pragma once

#include "App.h"

class VolumeApp : public App {
public:
    void handlePreLoop(WNDPROC winProc);
    void shutDown(HWND hWnd);
    void setWindowSemiTransparent(bool semiTransparent);

    virtual void onMouseLeave() override;
    virtual void onMouseEnter() override;
};
