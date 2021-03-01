#include "pch.h"

#include "App.h"

std::shared_ptr<App> g_pApp;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR pCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);

    g_pApp = std::shared_ptr<App>(new App());

    if (FAILED(g_pApp->CreateDesktopWindow(hInstance, nCmdShow, WndProc)))
        return 0;

    if (FAILED(g_pApp->CreateDeviceResources()))
        return 0;

    g_pApp->Run();

    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;

    switch (uMsg)
    {
    case WM_SIZE:
        hr = g_pApp->OnResize();
        if (FAILED(hr))
            PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        return g_pApp->KeyHandler(wParam, lParam);
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
        return g_pApp->MouseHandler(uMsg, wParam);
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}
