#ifndef UNICODE
#define UNICODE
#endif 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h> 
#include <directxmath.h>


struct Vertex
{
    DirectX::XMFLOAT3 Pos;
};


struct Transformation
{
    DirectX::XMMATRIX mWorld;
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
};


HWND g_hwnd = nullptr;
D3D_DRIVER_TYPE g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11InputLayout* g_pInputLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;
ID3D11Buffer* g_pTransformBuffer = nullptr;
ID3D11Buffer* g_pIndexBuffer = nullptr;
DirectX::XMMATRIX g_World;
DirectX::XMMATRIX g_View;
DirectX::XMMATRIX g_Projection;


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render();


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(pCmdLine);

    if (FAILED(InitWindow(hInstance, nCmdShow)))
        return 0;

    if (FAILED(InitDevice()))
    {
        CleanupDevice();
        return 0;
    }

    // Run the message loop.

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

    CleanupDevice();

    return 0;
}


HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"Lab1Class";
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
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
    g_hwnd = CreateWindow(CLASS_NAME, L"Lab1", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    if (!g_hwnd)
        return E_FAIL;

    ShowWindow(g_hwnd, nCmdShow);

    return S_OK;
}


HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName,nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel, 
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr)) 
    {
        if (pErrorBlob)
        {
            OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
            pErrorBlob->Release();
        }
        return hr;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return S_OK;
}


HRESULT CreateViews(UINT width, UINT height)
{
    // Create a render target view
    ID3D11Texture2D* pBackBuffer;
    HRESULT hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr))
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return hr;

    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pd3dDeviceContext->RSSetViewports(1, &vp);

    return S_OK;
}


HRESULT CreateShaders()
{
    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pPSBlob = nullptr;

    HRESULT hr = CompileShaderFromFile(L"shaders.hlsl", "vs_main", "vs_5_0", &pVSBlob);
    if (FAILED(hr))
        return hr;

    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pInputLayout);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }

    // Set the input layout
    g_pd3dDeviceContext->IASetInputLayout(g_pInputLayout);

    hr = CompileShaderFromFile(L"shaders.hlsl", "ps_main", "ps_5_0", &pPSBlob);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    if (FAILED(hr))
    {
        pVSBlob->Release();
        pPSBlob->Release();
        return hr;
    }

    return S_OK;
}


void InitMatrices(UINT width, UINT height)
{
    // Initialize the world matrix
    g_World = DirectX::XMMatrixIdentity();
    
    // Initialize the view matrix
    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, 1.5f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    g_View = DirectX::XMMatrixLookAtLH(Eye, At, Up);
    
    // Initialize the projection matrix
    g_Projection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, width / (FLOAT)height, 0.01f, 100.0f);
}


HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(g_hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	// Debug layer enabling
	UINT createDeviceFlags = 0;
#if defined(_DEBUG)
	// If the project is in a debug build, enable the debug layer.
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pd3dDeviceContext);

        if (SUCCEEDED(hr))
            break;
    }
    if (FAILED(hr))
        return hr;

    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    {
        IDXGIDevice* dxgiDevice = nullptr;
        hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(hr))
        {
            IDXGIAdapter* adapter = nullptr;
            hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr))
            {
                hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
            }
            dxgiDevice->Release();
        }
    }
    if (FAILED(hr))
        return hr;

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);

    dxgiFactory->MakeWindowAssociation(g_hwnd, DXGI_MWA_NO_ALT_ENTER);
    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    hr = CreateViews(width, height);
    if (FAILED(hr))
        return hr;

    // Create the vertex and pixel shaders
    hr = CreateShaders();
    if (FAILED(hr))
        return hr;

    // Create vertex buffer
    Vertex vertices[] =
    {
        DirectX::XMFLOAT3( 0.0f,  0.5f,  0.0f),
        DirectX::XMFLOAT3( 0.5f, -0.5f,  0.0f),
        DirectX::XMFLOAT3(-0.5f, -0.5f,  0.0f),
    };
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * 3;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
    if (FAILED(hr))
        return hr;
    
    // Create index buffer
    WORD indices[] =
    {
        0, 1, 2,
        2, 1, 0,
    };
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * 6;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
    if (FAILED(hr))
        return hr;
    
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Transformation);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pTransformBuffer);
    if (FAILED(hr))
        return hr;
    
    InitMatrices(width, height);
    
    return S_OK;
}


void CleanupDevice()
{
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->ClearState();
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
    if (g_pTransformBuffer) g_pTransformBuffer->Release();
    if (g_pIndexBuffer) g_pIndexBuffer->Release();
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pd3dDeviceContext) g_pd3dDeviceContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;
    switch (uMsg)
    {
    case WM_SIZE:
        if (g_pSwapChain)
        {
            g_pd3dDeviceContext->OMSetRenderTargets(0, 0, 0);
            if (g_pRenderTargetView) g_pRenderTargetView->Release();
            hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            if (SUCCEEDED(hr))
                hr = CreateViews(LOWORD(lParam), HIWORD(lParam));
            if (FAILED(hr))
                PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void Render()
{
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
    
    // Clear the back buffer 
    float background_colour[4] = { 0.3f, 0.5f, 0.7f, 1.0f };
    g_pd3dDeviceContext->ClearRenderTargetView(g_pRenderTargetView, background_colour);

    // Set vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    
    // Set index buffer
    g_pd3dDeviceContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    
    // Set primitive topology
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    static float t = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0)
        timeStart = timeCur;
    t = (timeCur - timeStart) / 1000.0f;
    g_World = DirectX::XMMatrixRotationY(t);
    
    Transformation tr;
    tr.mWorld = XMMatrixTranspose(g_World);
    tr.mView = XMMatrixTranspose(g_View);
    tr.mProjection = XMMatrixTranspose(g_Projection);
    g_pd3dDeviceContext->UpdateSubresource(g_pTransformBuffer, 0, nullptr, &tr, 0, 0);
    
    // Render a triangle
    g_pd3dDeviceContext->VSSetShader(g_pVertexShader, nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &g_pTransformBuffer);
    g_pd3dDeviceContext->PSSetShader(g_pPixelShader, nullptr, 0);
    g_pd3dDeviceContext->DrawIndexed(6, 0, 0);

    // Present the information rendered to the back buffer to the front buffer (the screen)
    g_pSwapChain->Present(0, 0);
}
