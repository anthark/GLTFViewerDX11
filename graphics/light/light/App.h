#pragma once

#include "DeviceResources.h"
#include "Renderer.h"

class App
{
public:
    App();
    ~App();

    HRESULT CreateDesktopWindow(HINSTANCE hInstance, int nCmdShow, WNDPROC pWndProc);
    HRESULT CreateDeviceResources();

    HWND GetWindowHandle() const { return m_hWnd; };

    void    Run();
    HRESULT OnResize();

private:
    void Render();

    HWND m_hWnd;
    
    std::shared_ptr<DeviceResources> m_pDeviceResources;
    std::shared_ptr<Renderer>        m_pRenderer;
};
