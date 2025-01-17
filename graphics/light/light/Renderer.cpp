#include "pch.h"

#include "Renderer.h"
#include "DDSTextureLoader11.h"
#include "Utils.h"

Renderer::Renderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<Camera>& camera) :
    m_pDeviceResources(deviceResources),
    m_pCamera(camera),
    m_frameCount(0),
    m_indexCount(0),
    m_constantBufferData(),
    m_lightColorBufferData(),
    m_lightPositionBufferData()
{};

HRESULT Renderer::CreateShaders()
{
    HRESULT hr = S_OK;

    std::vector<BYTE> bytes;
    ID3D11Device* device = m_pDeviceResources->GetDevice();

    // Create the vertex shader
    hr = CreateVertexShader(device, L"VertexShader.cso", bytes, &m_pVertexShader);
    if (FAILED(hr))
        return hr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = m_pDeviceResources->GetDevice()->CreateInputLayout(layout, numElements, bytes.data(), bytes.size(), &m_pInputLayout);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = CreatePixelShader(device, L"PixelShader.cso", bytes, &m_pPixelShader);
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
        {DirectX::XMFLOAT3(-256.0f, 0.0f, -144.0f), DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
        {DirectX::XMFLOAT3(-256.0f, 0.0f, 144.0f), DirectX::XMFLOAT2(0.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
        {DirectX::XMFLOAT3(256.0f, 0.0f, 144.0f), DirectX::XMFLOAT2(1.0f, 1.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
        {DirectX::XMFLOAT3(256.0f, 0.0f, -144.0f), DirectX::XMFLOAT2(1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f)},
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

    hr = DirectX::CreateDDSTextureFromFileEx(m_pDeviceResources->GetDevice(), nullptr, L"stone.dds", 0,
        D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, nullptr, &m_pTexture);
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
        DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, m_pDeviceResources->GetAspectRatio(), 0.01f, 1000.0f)
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
    if (FAILED(hr))
        return hr;

    hr = CreateLights();
    if (FAILED(hr))
        return hr;

    m_pToneMap = std::unique_ptr<ToneMapPostProcess>(new ToneMapPostProcess());
    hr = m_pToneMap->CreateDeviceDependentResources(m_pDeviceResources->GetDevice());
    
    return hr;
}

HRESULT Renderer::CreateLights()
{
    HRESULT hr = S_OK;

    DirectX::XMFLOAT4 LightPositions[NUM_LIGHTS] =
    {
        DirectX::XMFLOAT4(4.0f, 10.0f, -5.0f, 1.0f),
        DirectX::XMFLOAT4(-4.0f, 10.0f, -5.0f, 1.0f),
        DirectX::XMFLOAT4(0.0f, 10.0f, 2.0f, 1.0f)
    };

    DirectX::XMFLOAT4 LightColors[NUM_LIGHTS] =
    {
        DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f),
        DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)
    };

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        m_lightColorBufferData.LightColor[i] = LightColors[i];
        m_lightPositionBufferData.LightPosition[i] = LightPositions[i];
    }

    // Create the constant buffers for lights
    CD3D11_BUFFER_DESC lpbd(
        sizeof(LightPositionConstantBuffer),
        D3D11_BIND_CONSTANT_BUFFER
    );
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&lpbd, nullptr, &m_pLightPositionBuffer);
    if (FAILED(hr))
        return hr;

    CD3D11_BUFFER_DESC lcbd(
        sizeof(LightColorConstantBuffer),
        D3D11_BIND_CONSTANT_BUFFER
    );
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&lcbd, nullptr, &m_pLightColorBuffer);

    return hr;
}

void Renderer::UpdateLightColor(UINT index, float factor)
{
    if (index < NUM_LIGHTS)
    {
        m_lightColorBufferData.LightColor[index].x *= factor;
        m_lightColorBufferData.LightColor[index].y *= factor;
        m_lightColorBufferData.LightColor[index].z *= factor;
    }
}

HRESULT Renderer::CreateWindowSizeDependentResources()
{
    HRESULT hr = S_OK;

    m_constantBufferData.World = DirectX::XMMatrixIdentity();

    UpdatePerspective();

    m_pRenderTexture = std::unique_ptr<RenderTexture>(new RenderTexture(DXGI_FORMAT_R32G32B32A32_FLOAT));
    hr = m_pRenderTexture->CreateResources(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetWidth(), m_pDeviceResources->GetHeight());
    if (FAILED(hr))
        return hr;

    hr = m_pToneMap->CreateWindowSizeDependentResources(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetWidth(), m_pDeviceResources->GetHeight());

    return hr;
}

HRESULT Renderer::OnResize()
{
    HRESULT hr = S_OK;

    UpdatePerspective();

    hr = m_pRenderTexture->CreateResources(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetWidth(), m_pDeviceResources->GetHeight());
    if (FAILED(hr))
        return hr;

    hr = m_pToneMap->CreateWindowSizeDependentResources(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetWidth(), m_pDeviceResources->GetHeight());

    return hr;
}

void Renderer::Update()
{
    m_frameCount++;

    if (m_frameCount == MAXUINT)
        m_frameCount = 0;
}

void Renderer::Clear()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    float backgroundColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(m_pRenderTexture->GetRenderTargetView(), backgroundColour);
    context->ClearRenderTargetView(m_pDeviceResources->GetRenderTarget(), backgroundColour);
    context->ClearDepthStencilView(m_pDeviceResources->GetDepthStencil(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Renderer::RenderInTexture()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
    ID3D11RenderTargetView* renderTarget = m_pRenderTexture->GetRenderTargetView();
    ID3D11DepthStencilView* depthStencil = m_pDeviceResources->GetDepthStencil();

    D3D11_VIEWPORT viewport = m_pRenderTexture->GetViewPort();

    context->OMSetRenderTargets(1, &renderTarget, depthStencil);
    context->RSSetViewports(1, &viewport);

    // Set vertex buffer
    UINT stride = sizeof(VertexData);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

    // Set index buffer
    context->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &m_constantBufferData, 0, 0);
    context->UpdateSubresource(m_pLightPositionBuffer.Get(), 0, nullptr, &m_lightPositionBufferData, 0, 0);
    context->UpdateSubresource(m_pLightColorBuffer.Get(), 0, nullptr, &m_lightColorBufferData, 0, 0);

    context->IASetInputLayout(m_pInputLayout.Get());

    // Render a triangle
    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->VSSetConstantBuffers(1, 1, m_pLightPositionBuffer.GetAddressOf());
    context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
    context->PSSetConstantBuffers(2, 1, m_pLightColorBuffer.GetAddressOf());
    context->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());
    context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
    
    context->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::PostProcessTexture()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    m_pToneMap->Process(context, m_pRenderTexture->GetShaderResourceView(), m_pDeviceResources->GetRenderTarget(), m_pDeviceResources->GetViewPort());
}

void Renderer::Render()
{
    m_constantBufferData.View = DirectX::XMMatrixTranspose(m_pCamera->GetViewMatrix());

    Clear();

    RenderInTexture();

    PostProcessTexture();
}

Renderer::~Renderer()
{}
