#include "pch.h"

#include "App.h"

App::App() :
    m_hWnd(),
    m_cursor()
{};

HRESULT App::CreateDesktopWindow(HINSTANCE hInstance, int nCmdShow, WNDPROC pWndProc)
{
    const wchar_t CLASS_NAME[] = L"Lab3Class";
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = pWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = CLASS_NAME;
    if (!RegisterClassEx(&wcex))
        return E_FAIL;

    // Create window
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    m_hWnd = CreateWindow(CLASS_NAME, L"Lab3", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!m_hWnd)
        return HRESULT_FROM_WIN32(GetLastError());

    ShowWindow(m_hWnd, nCmdShow);

    return S_OK;
}

LRESULT App::KeyHandler(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case 87: // W
        m_pCamera->MoveDirection(1.0f);
        break;
    case 65: // A
        m_pCamera->MovePerpendicular(1.0f);
        break;
    case 83: // S
        m_pCamera->MoveDirection(-1.0f);
        break;
    case 68: // D
        m_pCamera->MovePerpendicular(-1.0f);
        break;
    case 81: // Q
        m_pRenderer->UpdateLightColor(0, 10.0f);
        break;
    case 69: // E
        m_pRenderer->UpdateLightColor(0, 0.1f);
        break;
    default:
        break;
    }
    return 0;
}

LRESULT App::MouseHandler(UINT uMsg, WPARAM wParam)
{
    POINT currPos;
    int dx, dy;
    switch (uMsg)
    {
    case WM_LBUTTONDOWN:
        GetCursorPos(&m_cursor);
        break;
    case WM_MOUSEMOVE:
        if (wParam == MK_LBUTTON)
        {
            GetCursorPos(&currPos);
            dx = currPos.x - m_cursor.x;
            dy = currPos.y - m_cursor.y;
            m_cursor = currPos;
            m_pCamera->Rotate(dx * 5e-3f, dy * 5e-3f);
        }
        break;
    default:
        break;
    }
    return 0;
}

HRESULT App::CreateDeviceResources()
{
    HRESULT hr = S_OK;

    m_pCamera = std::shared_ptr<Camera>(new Camera());

    m_pDeviceResources = std::shared_ptr<DeviceResources>(new DeviceResources());
    hr = m_pDeviceResources->CreateDeviceResources();
    if (FAILED(hr))
        return hr;

    hr = m_pDeviceResources->CreateWindowResources(m_hWnd);
    if (FAILED(hr))
        return hr;

    m_pRenderer = std::shared_ptr<Renderer>(new Renderer(m_pDeviceResources, m_pCamera));
    hr = m_pRenderer->CreateDeviceDependentResources();
    if (FAILED(hr))
        return hr;

    hr = m_pRenderer->CreateWindowSizeDependentResources();

    return hr;
}

void App::Render()
{
    m_pDeviceResources->GetAnnotation()->BeginEvent(L"Start rendering");

    m_pRenderer->Update();
    m_pRenderer->Render();

    m_pDeviceResources->Present();

    m_pDeviceResources->GetAnnotation()->EndEvent();
}

void App::Run()
{
    MSG msg = { 0 };
    while (WM_QUIT != msg.message)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render();
        }
    }
}

HRESULT App::OnResize()
{
    HRESULT hr = S_OK;

    if (m_pDeviceResources && m_pDeviceResources->GetSwapChain())
    {
        hr = m_pDeviceResources->OnResize();
        if (SUCCEEDED(hr))
            hr = m_pRenderer->OnResize();
    }

    return hr;
}

App::~App()
{}
