#include "pch.h"

#include <fstream>

#include "Renderer.h"
#include "DDSTextureLoader11.h"

Renderer::Renderer(const std::shared_ptr<DeviceResources>& deviceResources) :
    m_pDeviceResources(deviceResources),
    m_frameCount(0),
    m_indexCount(0),
    m_constantBufferData()
{};

HRESULT Renderer::ReadCompiledShader(const WCHAR* szFileName, BYTE** bytes, size_t& bufferSize)
{
    std::ifstream csoFile(szFileName, std::ios::in | std::ios::binary | std::ios::ate);

    if (csoFile.is_open())
    {
        bufferSize = (size_t)csoFile.tellg();
        *bytes = new BYTE[bufferSize];

        csoFile.seekg(0, std::ios::beg);
        csoFile.read(reinterpret_cast<char*>(*bytes), bufferSize);
        csoFile.close();

        return S_OK;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

HRESULT Renderer::CreateShaders()
{
    HRESULT hr = S_OK;

    BYTE* bytes = nullptr;
    size_t bufferSize;

    // Read the vertex shader
    hr = ReadCompiledShader(L"VertexShader.cso", &bytes, bufferSize);
    if (FAILED(hr))
        return hr;

    // Create the vertex shader
    hr = m_pDeviceResources->GetDevice()->CreateVertexShader(bytes, bufferSize, nullptr, &m_pVertexShader);
    if (FAILED(hr))
    {
        delete[] bytes;
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = m_pDeviceResources->GetDevice()->CreateInputLayout(layout, numElements, bytes, bufferSize, &m_pInputLayout);
    delete[] bytes;
    if (FAILED(hr))
        return hr;

    // Read the pixel shader
    hr = ReadCompiledShader(L"PixelShader.cso", &bytes, bufferSize);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = m_pDeviceResources->GetDevice()->CreatePixelShader(bytes, bufferSize, nullptr, &m_pPixelShader);
    delete[] bytes;
    if (FAILED(hr))
        return hr;

    // Create the constant buffer for world-view-projection matrices
    CD3D11_BUFFER_DESC cbd(
        sizeof(WorldViewProjectionConstantBuffer),
        D3D11_BIND_CONSTANT_BUFFER
    );
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);

    return hr;
}

HRESULT Renderer::CreateRectangle()
{
    HRESULT hr = S_OK;

    // Create vertex buffer
    VertexData vertices[] =
    {
        {DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f)},
        {DirectX::XMFLOAT3(-0.5f, -0.5f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f)},
        {DirectX::XMFLOAT3(0.5f, -0.5f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f)},
        {DirectX::XMFLOAT3(0.5f, -0.5f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f)},
    };
    CD3D11_BUFFER_DESC vbd(sizeof(VertexData) * ARRAYSIZE(vertices), D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    initData.pSysMem = vertices;
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&vbd, &initData, &m_pVertexBuffer);
    if (FAILED(hr))
        return hr;

    // Create index buffer
    WORD indices[] =
    {
        0, 1, 2,
        2, 3, 0,
    };
    m_indexCount = ARRAYSIZE(indices);
    CD3D11_BUFFER_DESC ibd(sizeof(indices) * m_indexCount, D3D11_BIND_INDEX_BUFFER);
    initData.pSysMem = indices;
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&ibd, &initData, &m_pIndexBuffer);

    return hr;
}

HRESULT Renderer::CreateTexture()
{
    HRESULT hr = S_OK;

    hr = DirectX::CreateDDSTextureFromFile(m_pDeviceResources->GetDevice(), L"stone.dds", nullptr, &m_pTexture);
    if (FAILED(hr))
        return hr;

    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pDeviceResources->GetDevice()->CreateSamplerState(&sd, &m_pSamplerLinear);

    return hr;
}

void Renderer::UpdatePerspective()
{
    m_constantBufferData.Projection = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, m_pDeviceResources->GetAspectRatio(), 0.01f, 100.0f)
    );
}

HRESULT Renderer::CreateDeviceDependentResources()
{
    HRESULT hr = S_OK;

    hr = CreateShaders();
    if (FAILED(hr))
        return hr;

    hr = CreateRectangle();
    if (FAILED(hr))
        return hr;

    hr = CreateTexture();
    
    return hr;
}

void Renderer::CreateWindowSizeDependentResources()
{
    m_constantBufferData.World = DirectX::XMMatrixIdentity();

    DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
    DirectX::XMVECTOR At = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_constantBufferData.View = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(Eye, At, Up));

    UpdatePerspective();
}

void Renderer::Update()
{
    m_frameCount++;

    if (m_frameCount == MAXUINT)
        m_frameCount = 0;
}

void Renderer::Render()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
    ID3D11RenderTargetView* renderTarget = m_pDeviceResources->GetRenderTarget();
    ID3D11DepthStencilView* depthStencil = m_pDeviceResources->GetDepthStencil();

    float background_colour[4] = { 0.3f, 0.5f, 0.7f, 1.0f };
    context->ClearRenderTargetView(renderTarget, background_colour);
    context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH, 1.0f, 0);

    m_pDeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Set vertex buffer
    UINT stride = sizeof(VertexData);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

    // Set index buffer
    context->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &m_constantBufferData, 0, 0);

    context->IASetInputLayout(m_pInputLayout.Get());

    // Render a triangle
    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());
    context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
    context->DrawIndexed(m_indexCount, 0, 0);
}

Renderer::~Renderer()
{}
