#include "pch.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <vector>

#include "Renderer.h"
#include "Utils.h"

const float sphereRadius = 0.5f;

Renderer::Renderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<Camera>& camera, const std::shared_ptr<Settings>& settings) :
    m_pDeviceResources(deviceResources),
    m_pCamera(camera),
    m_pSettings(settings),
    m_frameCount(0),
    m_indexCount(0),
    m_constantBufferData(),
    m_lightColorBufferData(),
    m_lightPositionBufferData(),
    m_materialBufferData()
{};

HRESULT Renderer::CreateShaders()
{
    HRESULT hr = S_OK;

    std::vector<BYTE> bytes;
    ID3D11Device* device = m_pDeviceResources->GetDevice();

    // Create the vertex shader
    hr = CreateVertexShader(device, L"PBRVertexShader.cso", bytes, &m_pVertexShader);
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
    hr = CreatePixelShader(device, L"PBRPixelShader.cso", bytes, &m_pPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for normal distribution function
    hr = CreatePixelShader(device, L"NDPixelShader.cso", bytes, &m_pNDPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for geometry function
    hr = CreatePixelShader(device, L"GPixelShader.cso", bytes, &m_pGPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for fresnel function
    hr = CreatePixelShader(device, L"FPixelShader.cso", bytes, &m_pFPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the constant buffer for world-view-projection matrices
    CD3D11_BUFFER_DESC cbd(
        sizeof(WorldViewProjectionConstantBuffer),
        D3D11_BIND_CONSTANT_BUFFER
    );
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);
    if (FAILED(hr))
        return hr;

    // Create the constant buffer for material variables
    CD3D11_BUFFER_DESC cbmd(
        sizeof(MaterialConstantBuffer),
        D3D11_BIND_CONSTANT_BUFFER
    );
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&cbmd, nullptr, &m_pMaterialBuffer);

    return hr;
}

HRESULT Renderer::CreateSphere()
{
    HRESULT hr = S_OK;

    const int numLines = 16;
    const float spacing = 1.0f / numLines;

    // Create vertex buffer
    std::vector<VertexData> vertices;
    for (int latitude = 0; latitude <= numLines; latitude++)
    {
        for (int longitude = 0; longitude <= numLines; longitude++)
        {
            VertexData v;

            v.Tex = DirectX::XMFLOAT2(longitude * spacing, 1.0f - latitude * spacing);

            float theta = v.Tex.x * 2.0f * static_cast<float>(M_PI);
            float phi = (v.Tex.y - 0.5f) * static_cast<float>(M_PI);
            float c = static_cast<float>(cos(phi));

            v.Normal = DirectX::XMFLOAT3(
                c * static_cast<float>(cos(theta)) * sphereRadius,
                    static_cast<float>(sin(phi)) * sphereRadius,
                c * static_cast<float>(sin(theta)) * sphereRadius
            );
            v.Pos = DirectX::XMFLOAT3(v.Normal.x * sphereRadius, v.Normal.y * sphereRadius, v.Normal.z * sphereRadius);

            vertices.push_back(v);
        }
    }

    CD3D11_BUFFER_DESC vbd(sizeof(VertexData) * static_cast<UINT>(vertices.size()), D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    initData.pSysMem = vertices.data();
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&vbd, &initData, &m_pVertexBuffer);
    if (FAILED(hr))
        return hr;

    // Create index buffer
    std::vector<WORD> indices;
    for (int latitude = 0; latitude < numLines; latitude++)
    {
        for (int longitude = 0; longitude < numLines; longitude++)
        {
            indices.push_back(latitude * (numLines + 1) + longitude);
            indices.push_back((latitude + 1) * (numLines + 1) + longitude);
            indices.push_back(latitude * (numLines + 1) + (longitude + 1));

            indices.push_back(latitude * (numLines + 1) + (longitude + 1));
            indices.push_back((latitude + 1) * (numLines + 1) + longitude);
            indices.push_back((latitude + 1) * (numLines + 1) + (longitude + 1));
        }
    }

    m_indexCount = static_cast<UINT32>(indices.size());
    CD3D11_BUFFER_DESC ibd(sizeof(WORD) * m_indexCount, D3D11_BIND_INDEX_BUFFER);
    initData.pSysMem = indices.data();
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&ibd, &initData, &m_pIndexBuffer);

    return hr;
}

void Renderer::UpdatePerspective()
{
    m_constantBufferData.Projection = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixPerspectiveFovRH(DirectX::XM_PIDIV2, m_pDeviceResources->GetAspectRatio(), 0.01f, 1000.0f)
    );
}

HRESULT Renderer::CreateDeviceDependentResources()
{
    HRESULT hr = S_OK;

    hr = CreateShaders();
    if (FAILED(hr))
        return hr;

    hr = CreateSphere();
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
        DirectX::XMFLOAT4(0.0f, 0.0f, 3.0f, 1.0f),
        DirectX::XMFLOAT4(0.0f, 0.0f, 3.0f, 1.0f),
        DirectX::XMFLOAT4(0.0f, 0.0f, 3.0f, 1.0f)
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
        m_lightColorBufferData.LightColor[index].w *= factor;
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
    
    m_constantBufferData.View = DirectX::XMMatrixTranspose(m_pCamera->GetViewMatrix());
    DirectX::XMStoreFloat4(&m_constantBufferData.CameraPos, m_pCamera->GetPosition());
}

void Renderer::Clear()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    float backgroundColour[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    context->ClearRenderTargetView(m_pRenderTexture->GetRenderTargetView(), backgroundColour);
    context->ClearRenderTargetView(m_pDeviceResources->GetRenderTarget(), backgroundColour);
    context->ClearDepthStencilView(m_pDeviceResources->GetDepthStencil(), D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void Renderer::RenderInTexture(ID3D11RenderTargetView* renderTarget)
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
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

    context->UpdateSubresource(m_pLightPositionBuffer.Get(), 0, nullptr, &m_lightPositionBufferData, 0, 0);
    context->UpdateSubresource(m_pLightColorBuffer.Get(), 0, nullptr, &m_lightColorBufferData, 0, 0);

    context->IASetInputLayout(m_pInputLayout.Get());

    // Render spheres
    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(1, 1, m_pLightPositionBuffer.GetAddressOf());

    switch (m_pSettings->GetShaderMode())
    {
    case Settings::PBRShaderMode::REGULAR:
        context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
        break;
    case Settings::PBRShaderMode::NORMAL_DISTRIBUTION:
        context->PSSetShader(m_pNDPixelShader.Get(), nullptr, 0);
        break;
    case Settings::PBRShaderMode::GEOMETRY:
        context->PSSetShader(m_pGPixelShader.Get(), nullptr, 0);
        break;
    case Settings::PBRShaderMode::FRESNEL:
        context->PSSetShader(m_pFPixelShader.Get(), nullptr, 0);
        break;
    default:
        context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
        break;
    }
    
    context->PSSetConstantBuffers(2, 1, m_pLightColorBuffer.GetAddressOf());

    const int sphereGridSize = 10;
    const float gridWidth = 5;
    m_materialBufferData.Albedo = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
    for (int i = 0; i < sphereGridSize; i++)
    {
        for (int j = 0; j < sphereGridSize; j++)
        {
            m_constantBufferData.World = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(
                gridWidth * (i / (sphereGridSize - 1.0f) - sphereRadius),
                gridWidth * (j / (sphereGridSize - 1.0f) - sphereRadius),
                0
            ));
            context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &m_constantBufferData, 0, 0);
            m_materialBufferData.Roughness = i / (sphereGridSize - 1.0f);
            m_materialBufferData.Metalness = j / (sphereGridSize - 1.0f);
            context->UpdateSubresource(m_pMaterialBuffer.Get(), 0, nullptr, &m_materialBufferData, 0, 0);
            context->PSSetConstantBuffers(3, 1, m_pMaterialBuffer.GetAddressOf());
            context->DrawIndexed(m_indexCount, 0, 0);
        }
    }
}

void Renderer::PostProcessTexture()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    m_pToneMap->Process(context, m_pRenderTexture->GetShaderResourceView(), m_pDeviceResources->GetRenderTarget(), m_pDeviceResources->GetViewPort());
}

void Renderer::Render()
{
    Clear();

    if (m_pSettings->GetShaderMode() == Settings::PBRShaderMode::REGULAR)
    {
        RenderInTexture(m_pRenderTexture->GetRenderTargetView());
        PostProcessTexture();
    }
    else
    {
        RenderInTexture(m_pDeviceResources->GetRenderTarget());
    }
}

Renderer::~Renderer()
{}
