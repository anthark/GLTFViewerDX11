#include "pch.h"

#define _USE_MATH_DEFINES

#include <math.h>
#include <vector>

#include "Renderer.h"
#include "Utils.h"

#include "../../stb_image.h"
#include "../../DDSTextureLoader11.h"

const float sphereRadius = 0.5f;
const UINT cubeSize = 512;
const UINT irradianceSize = 32;
const UINT prefilteredColorSize = 128;
const UINT preintegratedBRDFSize = 128;
const UINT simpleShadowMapSize = 1024;

Renderer::Renderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<Camera>& camera, const std::shared_ptr<Settings>& settings) :
    m_pDeviceResources(deviceResources),
    m_pCamera(camera),
    m_pSettings(settings),
    m_frameCount(0),
    m_indexCount(0),
    m_planeIndexCount(0),
    m_constantBufferData(),
    m_lightBufferData(),
    m_materialBufferData()
{};

HRESULT Renderer::CreateShaders()
{
    HRESULT hr = S_OK;

    std::vector<BYTE> bytes;
    ID3D11Device* device = m_pDeviceResources->GetDevice();

    // Create the vertex shader for environment
    hr = CreateVertexShader(device, L"EnvironmentVertexShader.cso", bytes, &m_pEnvironmentVertexShader);
    if (FAILED(hr))
        return hr;

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD_", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);

    // Create the input layout
    hr = m_pDeviceResources->GetDevice()->CreateInputLayout(layout, numElements, bytes.data(), bytes.size(), &m_pInputLayout);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for environment
    hr = CreatePixelShader(device, L"EnvironmentPixelShader.cso", bytes, &m_pEnvironmentPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the vertex shader
    hr = CreateVertexShader(device, L"PBRVertexShader.cso", bytes, &m_pPBRVertexShader);
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

    // Create the vertex shader for irradiance
    hr = CreateVertexShader(device, L"IBLVertexShader.cso", bytes, &m_pIBLVertexShader);
    if (FAILED(hr))
        return hr;

    // Define the input layout for IBL
    D3D11_INPUT_ELEMENT_DESC irradianceLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    numElements = ARRAYSIZE(irradianceLayout);

    // Create the input layout
    hr = m_pDeviceResources->GetDevice()->CreateInputLayout(irradianceLayout, numElements, bytes.data(), bytes.size(), &m_pIBLInputLayout);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for irradiance
    hr = CreatePixelShader(device, L"IrradiancePixelShader.cso", bytes, &m_pIrradiancePixelShader);
    if (FAILED(hr))
        return hr;

    // Create the vertex shader for environment cube
    hr = CreateVertexShader(device, L"EnvironmentCubeVertexShader.cso", bytes, &m_pEnvironmentCubeVertexShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for environment cube
    hr = CreatePixelShader(device, L"EnvironmentCubePixelShader.cso", bytes, &m_pEnvironmentCubePixelShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for prefiltered color
    hr = CreatePixelShader(device, L"PrefilteredColorPixelShader.cso", bytes, &m_pPrefilteredColorPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for preintegrated BRDF
    hr = CreatePixelShader(device, L"PreintegratedBRDFPixelShader.cso", bytes, &m_pPreintegratedBRDFPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader for plane
    D3D_SHADER_MACRO defines[] =
    {
        { "HAS_COLOR_TEXTURE", "1" },
        { nullptr, nullptr }
    };
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    hr = CompileShaderFromFile((wsrcPath + L"PBRShaders.fx").c_str(), "ps_main", "ps_5_0", &blob, defines);
    if (FAILED(hr))
        return hr;
    hr = device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_pPlanePixelShader);
    if (FAILED(hr))
        return hr;

    // Create the constant buffer for world-view-projection matrices
    CD3D11_BUFFER_DESC cbd(sizeof(WorldViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);
    if (FAILED(hr))
        return hr;

    // Create the constant buffer for material variables
    CD3D11_BUFFER_DESC cbmd(sizeof(MaterialConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&cbmd, nullptr, &m_pMaterialBuffer);

    return hr;
}

HRESULT Renderer::CreateSphere()
{
    HRESULT hr = S_OK;

    const int numLines = 32;
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
    if (FAILED(hr))
        return hr;

    // Create vertex buffer for environment sphere
    for (VertexData& v : vertices)
    {
        DirectX::XMStoreFloat3(&v.Pos, DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&v.Pos)));
        DirectX::XMStoreFloat3(&v.Normal, DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&v.Normal)));
    }
    initData.pSysMem = vertices.data();
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&vbd, &initData, &m_pEnvironmentVertexBuffer);

    return hr;
}

HRESULT Renderer::CreatePlane()
{
    HRESULT hr = S_OK;

    VertexData vertices[] =
    {
        {DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT3(-750.0f, 0.0f, -750.0f), DirectX::XMFLOAT2(0.0f, 0.0f)},
        {DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT3(-750.0f, 0.0f, 750.0f), DirectX::XMFLOAT2(0.0f, 30.0f)},
        {DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT3(750.0f, 0.0f, 750.0f), DirectX::XMFLOAT2(30.0f, 30.0f)},
        {DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f), DirectX::XMFLOAT3(750.0f, 0.0f, -750.0f), DirectX::XMFLOAT2(30.0f, 0.0f)}
    };

    CD3D11_BUFFER_DESC vbd(sizeof(VertexData) * ARRAYSIZE(vertices), D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    initData.pSysMem = vertices;
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&vbd, &initData, &m_pPlaneVertexBuffer);
    if (FAILED(hr))
        return hr;

    WORD indices[] =
    {
        0, 3, 2,
        2, 1, 0
    };

    m_planeIndexCount = ARRAYSIZE(indices);
    CD3D11_BUFFER_DESC ibd(sizeof(WORD) * m_planeIndexCount, D3D11_BIND_INDEX_BUFFER);
    initData.pSysMem = indices;
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&ibd, &initData, &m_pPlaneIndexBuffer);
    if (FAILED(hr))
        return hr;

    hr = DirectX::CreateDDSTextureFromFileEx(m_pDeviceResources->GetDevice(), nullptr, (wsrcPath + L"grass.dds").c_str(), 0,
        D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, true, nullptr, &m_pPlaneShaderResourceView);

    return hr;
}

HRESULT Renderer::CreateTexture()
{
    HRESULT hr = S_OK;

    int w, h, n;
    float* data = stbi_loadf("env.hdr", &w, &h, &n, STBI_rgb_alpha);
    if (data == nullptr)
        return E_FAIL;

    ID3D11Device* device = m_pDeviceResources->GetDevice();

    CD3D11_TEXTURE2D_DESC td(DXGI_FORMAT_R32G32B32A32_FLOAT, w, h, 1, 1, D3D11_BIND_SHADER_RESOURCE);
    D3D11_SUBRESOURCE_DATA initData;
    initData.pSysMem = data;
    initData.SysMemPitch = 4 * w * sizeof(float);
    hr = device->CreateTexture2D(&td, &initData, &m_pEnvironmentTexture);
    stbi_image_free(data);
    if (FAILED(hr))
        return hr;

    CD3D11_SHADER_RESOURCE_VIEW_DESC srvd(D3D11_SRV_DIMENSION_TEXTURE2D, td.Format);
    hr = device->CreateShaderResourceView(m_pEnvironmentTexture.Get(), &srvd, &m_pEnvironmentShaderResourceView);
    if (FAILED(hr))
        return hr;

    m_pSamplerStates.resize(3);
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device->CreateSamplerState(&sd, &m_pSamplerStates[0]);
    if (FAILED(hr))
        return hr;

    sd.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    hr = device->CreateSamplerState(&sd, &m_pSamplerStates[1]);
    if (FAILED(hr))
        return hr;

    sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_LESS;
    hr = device->CreateSamplerState(&sd, &m_pSamplerStates[2]);

    return hr;
}

HRESULT Renderer::CreateCubeTextureFromResource(UINT size, ID3D11Texture2D* dst, ID3D11ShaderResourceView* src, ID3D11VertexShader* vs, ID3D11PixelShader* ps, UINT mipSlice)
{
    HRESULT hr = S_OK;

    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

    VertexPosData vertices[4];
    WORD indices[6] =
    {
        0, 1, 2,
        2, 3, 0
    };
    UINT indexCount = 6;

    ID3D11Device* device = m_pDeviceResources->GetDevice();

    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    CD3D11_BUFFER_DESC vbd(sizeof(VertexPosData) * 4, D3D11_BIND_VERTEX_BUFFER);
    CD3D11_BUFFER_DESC ibd(sizeof(WORD) * indexCount, D3D11_BIND_INDEX_BUFFER);
    initData.pSysMem = indices;
    hr = device->CreateBuffer(&ibd, &initData, &indexBuffer);
    if (FAILED(hr))
        return hr;
 
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    D3D11_TEXTURE2D_DESC dtd;
    dst->GetDesc(&dtd);
    D3D11_TEXTURE2D_DESC td = CD3D11_TEXTURE2D_DESC(dtd.Format, size, size, 1, 1, D3D11_BIND_RENDER_TARGET);
    hr = device->CreateTexture2D(&td, nullptr, &texture);
    if (FAILED(hr))
        return hr;

    // Create render target view
    CD3D11_RENDER_TARGET_VIEW_DESC rtvd(D3D11_RTV_DIMENSION_TEXTURE2D, td.Format);
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTarget;
    hr = device->CreateRenderTargetView(texture.Get(), &rtvd, &pRenderTarget);
    if (FAILED(hr))
        return hr;

    D3D11_VIEWPORT viewport;
    viewport.Width = static_cast<FLOAT>(size);
    viewport.Height = static_cast<FLOAT>(size);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    WorldViewProjectionConstantBuffer cb;
    cb.World = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    cb.Projection = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1, 0.2f, 0.8f));

    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
    ID3D11RenderTargetView* renderTarget = pRenderTarget.Get();
    context->RSSetViewports(1, &viewport);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    UINT stride = sizeof(VertexPosData);
    UINT offset = 0;

    // Set index buffer
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_pIBLInputLayout.Get());

    // Render irradiance 
    context->VSSetShader(vs, nullptr, 0);
    context->PSSetShader(ps, nullptr, 0);
    context->PSSetShaderResources(0, 1, &src);
    context->PSSetSamplers(0, 1, m_pSamplerStates[0].GetAddressOf());
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

    float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    D3D11_BOX box = CD3D11_BOX(0, 0, 0, size, size, 1);
    for (UINT i = 0; i < 6; ++i)
    {
        UINT constCoord = i / 2;
        UINT widthCoord = i >= 2 ? 0 : 2;
        UINT heightCoord = (i == 2 || i == 3) ? 2 : 1;
        FLOAT leftBottomAngel[3] = { m_squareLeftBottomAngles[i].x, m_squareLeftBottomAngles[i].y, m_squareLeftBottomAngles[i].z };
        FLOAT rightTopAngel[3] = { m_squareRightTopAngles[i].x, m_squareRightTopAngles[i].y, m_squareRightTopAngles[i].z };
        FLOAT pos[3];
        pos[constCoord] = leftBottomAngel[constCoord];
        pos[widthCoord] = leftBottomAngel[widthCoord];
        pos[heightCoord] = leftBottomAngel[heightCoord];
        vertices[0].Pos = DirectX::XMFLOAT3(pos);
        pos[heightCoord] = rightTopAngel[heightCoord];
        vertices[1].Pos = DirectX::XMFLOAT3(pos);
        pos[widthCoord] = rightTopAngel[widthCoord];
        vertices[2].Pos = DirectX::XMFLOAT3(pos);
        pos[heightCoord] = leftBottomAngel[heightCoord];
        vertices[3].Pos = DirectX::XMFLOAT3(pos);

        initData.pSysMem = vertices;
        hr = device->CreateBuffer(&vbd, &initData, vertexBuffer.ReleaseAndGetAddressOf());
        if (FAILED(hr))
            return hr;

        context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
        cb.View = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0, 0, 0, 0), m_targers[i], m_ups[i]));
        context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
        context->ClearRenderTargetView(renderTarget, color);
        context->DrawIndexed(indexCount, 0, 0);
        context->CopySubresourceRegion(dst, D3D11CalcSubresource(mipSlice, i, dtd.MipLevels), 0, 0, 0, texture.Get(), 0, &box);
    }

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);

    return hr;
}

HRESULT Renderer::CreateCubeTexture()
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC td = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32G32B32A32_FLOAT, cubeSize, cubeSize, 6, 0, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
        D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS);
    hr = m_pDeviceResources->GetDevice()->CreateTexture2D(&td, nullptr, &m_pEnvironmentCubeTexture);
    if (FAILED(hr))
        return hr;

    hr = CreateCubeTextureFromResource(cubeSize, m_pEnvironmentCubeTexture.Get(), m_pEnvironmentShaderResourceView.Get(),
        m_pEnvironmentCubeVertexShader.Get(), m_pEnvironmentCubePixelShader.Get());
    if (FAILED(hr))
        return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURECUBE, td.Format);
    hr = m_pDeviceResources->GetDevice()->CreateShaderResourceView(m_pEnvironmentCubeTexture.Get(), &srvd, &m_pEnvironmentCubeShaderResourceView);
    if (FAILED(hr))
        return hr;

    m_pDeviceResources->GetDeviceContext()->GenerateMips(m_pEnvironmentCubeShaderResourceView.Get());
    return hr;
}

HRESULT Renderer::CreateIrradianceTexture()
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC td = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32G32B32A32_FLOAT, irradianceSize, irradianceSize, 6, 1, D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
    hr = m_pDeviceResources->GetDevice()->CreateTexture2D(&td, nullptr, &m_pIrradianceTexture);
    if (FAILED(hr))
        return hr;

    hr = CreateCubeTextureFromResource(irradianceSize, m_pIrradianceTexture.Get(), m_pEnvironmentCubeShaderResourceView.Get(),
        m_pIBLVertexShader.Get(), m_pIrradiancePixelShader.Get());
    if (FAILED(hr))
        return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURECUBE, td.Format, 0, 1);
    hr = m_pDeviceResources->GetDevice()->CreateShaderResourceView(m_pIrradianceTexture.Get(), &srvd, &m_pIrradianceShaderResourceView);

    return hr;
}

HRESULT Renderer::CreatePrefilteredColorTexture()
{
    HRESULT hr = S_OK;

    float roughness[5] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

    D3D11_TEXTURE2D_DESC td = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32G32B32A32_FLOAT, prefilteredColorSize, prefilteredColorSize, 6, ARRAYSIZE(roughness),
        D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
    hr = m_pDeviceResources->GetDevice()->CreateTexture2D(&td, nullptr, &m_pPrefilteredColorTexture);
    if (FAILED(hr))
        return hr;

    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
    context->PSSetConstantBuffers(1, 1, m_pMaterialBuffer.GetAddressOf());
    for (UINT i = 0; i < td.MipLevels; ++i)
    {
        m_materialBufferData.Roughness = roughness[i];
        context->UpdateSubresource(m_pMaterialBuffer.Get(), 0, nullptr, &m_materialBufferData, 0, 0);

        hr = CreateCubeTextureFromResource(prefilteredColorSize / (UINT)pow(2, i), m_pPrefilteredColorTexture.Get(), m_pEnvironmentCubeShaderResourceView.Get(),
            m_pIBLVertexShader.Get(), m_pPrefilteredColorPixelShader.Get(), i);
        if (FAILED(hr))
            return hr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURECUBE, td.Format, 0, td.MipLevels);
    hr = m_pDeviceResources->GetDevice()->CreateShaderResourceView(m_pPrefilteredColorTexture.Get(), &srvd, &m_pPrefilteredColorShaderResourceView);

    return hr;
}

HRESULT Renderer::CreatePreintegratedBRDFTexture()
{
    HRESULT hr = S_OK;

    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;

    VertexPosData vertices[4] =
    {
        {DirectX::XMFLOAT3(0.0f, 0.0f, 0.5f)},
        {DirectX::XMFLOAT3(0.0f, 1.0f, 0.5f)},
        {DirectX::XMFLOAT3(1.0f, 1.0f, 0.5f)},
        {DirectX::XMFLOAT3(1.0f, 0.0f, 0.5f)}
    };

    WORD indices[6] =
    {
        0, 1, 2,
        2, 3, 0
    };
    UINT indexCount = 6;

    ID3D11Device* device = m_pDeviceResources->GetDevice();

    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    CD3D11_BUFFER_DESC vbd(sizeof(VertexPosData) * 4, D3D11_BIND_VERTEX_BUFFER);
    initData.pSysMem = vertices;
    hr = device->CreateBuffer(&vbd, &initData, &vertexBuffer);
    if (FAILED(hr))
        return hr;

    CD3D11_BUFFER_DESC ibd(sizeof(WORD) * indexCount, D3D11_BIND_INDEX_BUFFER);
    initData.pSysMem = indices;
    hr = device->CreateBuffer(&ibd, &initData, &indexBuffer);
    if (FAILED(hr))
        return hr;

    D3D11_TEXTURE2D_DESC td = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32G32B32A32_FLOAT, preintegratedBRDFSize, preintegratedBRDFSize, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET);
    hr = device->CreateTexture2D(&td, nullptr, &m_pPreintegratedBRDFTexture);
    if (FAILED(hr))
        return hr;

    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
    CD3D11_RENDER_TARGET_VIEW_DESC rtvd(D3D11_RTV_DIMENSION_TEXTURE2D, td.Format);
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRenderTarget;
    hr = device->CreateRenderTargetView(m_pPreintegratedBRDFTexture.Get(), &rtvd, &pRenderTarget);
    if (FAILED(hr))
        return hr;

    D3D11_VIEWPORT viewport;
    viewport.Width = static_cast<FLOAT>(preintegratedBRDFSize);
    viewport.Height = static_cast<FLOAT>(preintegratedBRDFSize);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    WorldViewProjectionConstantBuffer cb;
    cb.World = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
    cb.Projection = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1, 0.2f, 0.8f));

    ID3D11RenderTargetView* renderTarget = pRenderTarget.Get();
    context->RSSetViewports(1, &viewport);
    context->OMSetRenderTargets(1, &renderTarget, nullptr);

    UINT stride = sizeof(VertexPosData);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_pIBLInputLayout.Get());

    context->VSSetShader(m_pIBLVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->PSSetShader(m_pPreintegratedBRDFPixelShader.Get(), nullptr, 0);

    float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cb.View = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(DirectX::XMVectorSet(0.5f, 0.5f, 0, 0), DirectX::XMVectorSet(0.5f, 0.5f, 1.0f, 0), DirectX::XMVectorSet(0, 1.0f, 0, 0)));
    context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    context->ClearRenderTargetView(renderTarget, color);
    context->DrawIndexed(indexCount, 0, 0);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, td.Format);
    hr = device->CreateShaderResourceView(m_pPreintegratedBRDFTexture.Get(), &srvd, &m_pPreintegratedBRDFShaderResourceView);

    return hr;
}

void Renderer::UpdatePerspective()
{
    m_constantBufferData.Projection = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixPerspectiveFovRH(DirectX::XM_PIDIV2, m_pDeviceResources->GetAspectRatio(), 0.1f, 10000.0f)
    );
}

HRESULT Renderer::CreateModels()
{
    HRESULT hr = S_OK;

    ID3D11Device* device = m_pDeviceResources->GetDevice();

    m_pModelShaders = std::shared_ptr<ModelShaders>(new ModelShaders());
    hr = m_pModelShaders->CreateDeviceDependentResources(device);
    if (FAILED(hr))
        return hr;

    DirectX::XMMATRIX translation;
    DirectX::XMMATRIX rotation;
    DirectX::XMMATRIX scale;

    translation = DirectX::XMMatrixTranslation(0, 0.5f, 1000);
    rotation = DirectX::XMMatrixRotationY(static_cast<float>(M_PI_2));
    scale = DirectX::XMMatrixScaling(0.12f, 0.12f, 0.12f);
    m_pModels.push_back(std::unique_ptr<Model>(new Model("car_scene/scene.gltf", m_pModelShaders,
        DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(rotation, translation), scale))));
    hr = m_pModels[0]->CreateDeviceDependentResources(device);
    if (FAILED(hr))
        return hr;

    translation = DirectX::XMMatrixTranslation(25, -5.43f, 10);
    rotation = DirectX::XMMatrixRotationY(static_cast<float>(-M_PI_2));
    scale = DirectX::XMMatrixScaling(10, 10, 10);
    m_pModels.push_back(std::unique_ptr<Model>(new Model("msz-006/scene.gltf", m_pModelShaders,
        DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(rotation, translation), scale))));
    hr = m_pModels[1]->CreateDeviceDependentResources(device);
    if (FAILED(hr))
        return hr;

    translation = DirectX::XMMatrixTranslation(-200, 300, 500);
    scale = DirectX::XMMatrixScaling(0.3f, 0.3f, 0.3f);
    m_pModels.push_back(std::unique_ptr<Model>(new Model("spitfire/scene.gltf", m_pModelShaders, DirectX::XMMatrixMultiply(translation, scale))));
    hr = m_pModels[2]->CreateDeviceDependentResources(device);
    if (FAILED(hr))
        return hr;

    translation = DirectX::XMMatrixTranslation(0, 0.566f, 0);
    scale = DirectX::XMMatrixScaling(100, 100, 100);
    m_pModels.push_back(std::unique_ptr<Model>(new Model("red_barn/scene.gltf", m_pModelShaders, DirectX::XMMatrixMultiply(translation, scale))));
    hr = m_pModels[3]->CreateDeviceDependentResources(device);

    return hr;
}

HRESULT Renderer::CreateShadows()
{
    HRESULT hr = S_OK;

    ID3D11Device* device = m_pDeviceResources->GetDevice();

    D3D11_TEXTURE2D_DESC dd = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32_TYPELESS, simpleShadowMapSize, simpleShadowMapSize, 1, 1, D3D11_BIND_DEPTH_STENCIL);
    hr = device->CreateTexture2D(&dd, nullptr, &m_pDepthSimpleShadowMapTexture);
    if (FAILED(hr))
        return hr;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
    hr = device->CreateDepthStencilView(m_pDepthSimpleShadowMapTexture.Get(), &dsvd, &m_pSimpleShadowMapDepthStencilView);
    if (FAILED(hr))
        return hr;

    D3D11_TEXTURE2D_DESC td = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32_TYPELESS, simpleShadowMapSize, simpleShadowMapSize, 6, 1, D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
    hr = device->CreateTexture2D(&td, nullptr, &m_pResourceSimpleShadowMapTexture);
    if (FAILED(hr))
        return hr;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURECUBE, DXGI_FORMAT_R32_FLOAT);
    hr = device->CreateShaderResourceView(m_pResourceSimpleShadowMapTexture.Get(), &srvd, &m_pSimpleShadowMapShaderResourceView);
    if (FAILED(hr))
        return hr;

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_BACK;
    rd.DepthBias = 0;
    rd.DepthBiasClamp = 0;
    rd.SlopeScaledDepthBias = 0;
    rd.DepthClipEnable = true;
    hr = device->CreateRasterizerState(&rd, &m_pSimpleShadowMapRasterizerState);

    return hr;
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

    hr = CreatePlane();
    if (FAILED(hr))
        return hr;

    hr = CreateTexture();
    if (FAILED(hr))
        return hr;

    hr = CreateCubeTexture();
    if (FAILED(hr))
        return hr;

    hr = CreateLights();
    if (FAILED(hr))
        return hr;

    hr = CreateIrradianceTexture();
    if (FAILED(hr))
       return hr;

    hr = CreatePrefilteredColorTexture();
    if (FAILED(hr))
        return hr;

    hr = CreatePreintegratedBRDFTexture();
    if (FAILED(hr))
        return hr;

    hr = CreateModels();
    if (FAILED(hr))
        return hr;

    m_pBloom = std::unique_ptr<BloomProcess>(new BloomProcess());
    hr = m_pBloom->CreateDeviceDependentResources(m_pDeviceResources->GetDevice());
    if (FAILED(hr))
        return hr;

    m_pToneMap = std::unique_ptr<ToneMapPostProcess>(new ToneMapPostProcess());
    hr = m_pToneMap->CreateDeviceDependentResources(m_pDeviceResources->GetDevice());
    if (FAILED(hr))
        return hr;

    hr = CreateShadows();
    
    return hr;
}

HRESULT Renderer::CreateLights()
{
    HRESULT hr = S_OK;

    CD3D11_BUFFER_DESC lbd(sizeof(LightConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
    hr = m_pDeviceResources->GetDevice()->CreateBuffer(&lbd, nullptr, &m_pLightBuffer);

    return hr;
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

    hr = m_pBloom->CreateWindowSizeDependentResources(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetWidth(), m_pDeviceResources->GetHeight());
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

    hr = m_pBloom->CreateWindowSizeDependentResources(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetWidth(), m_pDeviceResources->GetHeight());
    if (FAILED(hr))
        return hr;

    hr = m_pToneMap->CreateWindowSizeDependentResources(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetWidth(), m_pDeviceResources->GetHeight());

    return hr;
}

HRESULT Renderer::Update()
{
    HRESULT hr = S_OK;

    m_frameCount++;

    if (m_frameCount == MAXUINT)
        m_frameCount = 0;
    
    m_constantBufferData.View = DirectX::XMMatrixTranspose(m_pCamera->GetViewMatrix());
    DirectX::XMStoreFloat4(&m_constantBufferData.CameraPos, m_pCamera->GetPosition());

    for (UINT i = 0; i < NUM_LIGHTS; ++i)
    {
        m_lightBufferData.LightColor[i] = m_pSettings->GetLightColor(i);
        m_lightBufferData.LightPosition[i] = m_pSettings->GetLightPosition(i);
        m_lightBufferData.LightAttenuation[i] = m_pSettings->GetLightAttenuation(i);
    }

    m_lightBufferData.UseShadowPCF = m_pSettings->GetShadowPCFUsing();

    m_materialBufferData.Albedo = m_pSettings->GetAlbedo();
    m_materialBufferData.Roughness = m_pSettings->GetRoughness();
    m_materialBufferData.Metalness = m_pSettings->GetMetalness();

    D3D11_RASTERIZER_DESC rd;
    m_pSimpleShadowMapRasterizerState->GetDesc(&rd);
    int depthBias = m_pSettings->GetDepthBias();
    float slopeScaledDepthBias = m_pSettings->GetSlopeScaledDepthBias();
    if (depthBias != rd.DepthBias || slopeScaledDepthBias != rd.SlopeScaledDepthBias)
    {
        rd.DepthBias = depthBias;
        rd.SlopeScaledDepthBias = slopeScaledDepthBias;
        hr = m_pDeviceResources->GetDevice()->CreateRasterizerState(&rd, m_pSimpleShadowMapRasterizerState.ReleaseAndGetAddressOf());
    }

    return hr;
}

void Renderer::Clear()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    context->ClearState();

    float backgroundColour[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    context->ClearRenderTargetView(m_pRenderTexture->GetRenderTargetView(), backgroundColour);
    context->ClearRenderTargetView(m_pDeviceResources->GetRenderTarget(), backgroundColour);
    context->ClearDepthStencilView(m_pDeviceResources->GetDepthStencil(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    float blackColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    context->ClearRenderTargetView(m_pBloom->GetBloomRenderTargetView(), blackColour);
}

void Renderer::RenderSphere(WorldViewProjectionConstantBuffer& transformationData, bool usePS)
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    // Set vertex buffer
    UINT stride = sizeof(VertexData);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);

    // Set index buffer
    context->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->UpdateSubresource(m_pLightBuffer.Get(), 0, nullptr, &m_lightBufferData, 0, 0);

    context->IASetInputLayout(m_pInputLayout.Get());

    // Render spheres
    context->VSSetShader(m_pPBRVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

    if (usePS)
    {
        context->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, m_pLightBuffer.GetAddressOf());
        context->PSSetConstantBuffers(2, 1, m_pMaterialBuffer.GetAddressOf());
        context->PSSetShaderResources(0, 1, m_pIrradianceShaderResourceView.GetAddressOf());
        context->PSSetShaderResources(1, 1, m_pPrefilteredColorShaderResourceView.GetAddressOf());
        context->PSSetShaderResources(2, 1, m_pPreintegratedBRDFShaderResourceView.GetAddressOf());
        context->PSSetSamplers(0, 1, m_pSamplerStates[0].GetAddressOf());
        context->PSSetSamplers(1, 1, m_pSamplerStates[1].GetAddressOf());
        context->PSSetSamplers(3, 1, m_pSamplerStates[2].GetAddressOf());

        switch (m_pSettings->GetShaderMode())
        {
        case Settings::SETTINGS_PBR_SHADER_MODE::REGULAR:
            context->PSSetShaderResources(6, 1, m_pSimpleShadowMapShaderResourceView.GetAddressOf());
            context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
            break;
        case Settings::SETTINGS_PBR_SHADER_MODE::NORMAL_DISTRIBUTION:
            context->PSSetShader(m_pNDPixelShader.Get(), nullptr, 0);
            break;
        case Settings::SETTINGS_PBR_SHADER_MODE::GEOMETRY:
            context->PSSetShader(m_pGPixelShader.Get(), nullptr, 0);
            break;
        case Settings::SETTINGS_PBR_SHADER_MODE::FRESNEL:
            context->PSSetShader(m_pFPixelShader.Get(), nullptr, 0);
            break;
        default:
            context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
            break;
        }
    }

    transformationData.World = DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(100, 100, 100));
    context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &transformationData, 0, 0);
    context->UpdateSubresource(m_pMaterialBuffer.Get(), 0, nullptr, &m_materialBufferData, 0, 0);
    context->DrawIndexed(m_indexCount, 0, 0);
}

void Renderer::RenderEnvironment()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    // Set vertex buffer
    UINT stride = sizeof(VertexData);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_pEnvironmentVertexBuffer.GetAddressOf(), &stride, &offset);

    // Set index buffer
    context->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_pInputLayout.Get());

    m_constantBufferData.World = DirectX::XMMatrixMultiplyTranspose(
        DirectX::XMMatrixScaling(10000, 10000, 10000),
        DirectX::XMMatrixTranslationFromVector(m_pCamera->GetPosition())
    );
    context->UpdateSubresource(m_pConstantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

    // Render sphere
    context->VSSetShader(m_pEnvironmentVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->PSSetShader(m_pEnvironmentPixelShader.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, m_pEnvironmentCubeShaderResourceView.GetAddressOf());
    context->PSSetSamplers(0, 1, m_pSamplerStates[0].GetAddressOf());
    context->DrawIndexed(m_indexCount, 0, 0);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);
}

void Renderer::RenderPlane()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    // Set vertex buffer
    UINT stride = sizeof(VertexData);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_pPlaneVertexBuffer.GetAddressOf(), &stride, &offset);

    // Set index buffer
    context->IASetIndexBuffer(m_pPlaneIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

    // Set primitive topology
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_pInputLayout.Get());

    context->UpdateSubresource(m_pLightBuffer.Get(), 0, nullptr, &m_lightBufferData, 0, 0);

    m_constantBufferData.World = DirectX::XMMatrixIdentity();
    context->UpdateSubresource(m_pConstantBuffer.Get(), 0, NULL, &m_constantBufferData, 0, 0);

    m_materialBufferData.Albedo = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    m_materialBufferData.Roughness = 1.0f;
    m_materialBufferData.Metalness = 0.0f;
    context->UpdateSubresource(m_pMaterialBuffer.Get(), 0, nullptr, &m_materialBufferData, 0, 0);

    // Render plane
    context->VSSetShader(m_pPBRVertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
    context->PSSetConstantBuffers(1, 1, m_pLightBuffer.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_pMaterialBuffer.GetAddressOf());
    context->PSSetShaderResources(0, 1, m_pIrradianceShaderResourceView.GetAddressOf());
    context->PSSetShaderResources(1, 1, m_pPrefilteredColorShaderResourceView.GetAddressOf());
    context->PSSetShaderResources(2, 1, m_pPreintegratedBRDFShaderResourceView.GetAddressOf());
    context->PSSetShaderResources(3, 1, m_pPlaneShaderResourceView.GetAddressOf());
    context->PSSetShaderResources(6, 1, m_pSimpleShadowMapShaderResourceView.GetAddressOf());
    context->PSSetSamplers(0, 1, m_pSamplerStates[0].GetAddressOf());
    context->PSSetSamplers(1, 1, m_pSamplerStates[1].GetAddressOf());
    context->PSSetSamplers(2, 1, m_pSamplerStates[0].GetAddressOf());
    context->PSSetSamplers(3, 1, m_pSamplerStates[2].GetAddressOf());
    context->PSSetShader(m_pPlanePixelShader.Get(), nullptr, 0);
    context->DrawIndexed(m_planeIndexCount, 0, 0);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);
}

void Renderer::PostProcessTexture()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    m_pToneMap->Process(context, m_pRenderTexture->GetShaderResourceView(), m_pDeviceResources->GetRenderTarget(), m_pDeviceResources->GetViewPort());
}

void Renderer::RenderModels()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
    ID3D11RenderTargetView* renderTarget = m_pRenderTexture->GetRenderTargetView();
    ID3D11RenderTargetView* bloomRenderTarget = m_pBloom->GetBloomRenderTargetView();

    context->UpdateSubresource(m_pLightBuffer.Get(), 0, nullptr, &m_lightBufferData, 0, 0);
    context->PSSetConstantBuffers(1, 1, m_pLightBuffer.GetAddressOf());
    context->PSSetShaderResources(0, 1, m_pIrradianceShaderResourceView.GetAddressOf());
    context->PSSetShaderResources(1, 1, m_pPrefilteredColorShaderResourceView.GetAddressOf());
    context->PSSetShaderResources(2, 1, m_pPreintegratedBRDFShaderResourceView.GetAddressOf());
    context->PSSetShaderResources(6, 1, m_pSimpleShadowMapShaderResourceView.GetAddressOf());
    context->PSSetSamplers(0, 1, m_pSamplerStates[0].GetAddressOf());
    context->PSSetSamplers(1, 1, m_pSamplerStates[1].GetAddressOf());
    context->PSSetSamplers(3, 1, m_pSamplerStates[2].GetAddressOf());

    Model::ShadersSlots slots = { 3, 4, 5, 2, 0, 2 };
    context->OMSetRenderTargets(1, &renderTarget, m_pDeviceResources->GetDepthStencil());
    for (size_t i = 0; i < m_pModels.size(); ++i)
        m_pModels[i]->Render(context, m_constantBufferData, m_pConstantBuffer.Get(), m_pMaterialBuffer.Get(), slots);
    
    context->OMSetRenderTargets(1, &bloomRenderTarget, m_pDeviceResources->GetDepthStencil());
    for (size_t i = 0; i < m_pModels.size(); ++i)
        m_pModels[i]->Render(context, m_constantBufferData, m_pConstantBuffer.Get(), m_pMaterialBuffer.Get(), slots, true);
    
    context->OMSetRenderTargets(1, &renderTarget, m_pDeviceResources->GetDepthStencil());
    for (size_t i = 0; i < m_pModels.size(); ++i)
        m_pModels[i]->RenderTransparent(context, m_constantBufferData, m_pConstantBuffer.Get(), m_pMaterialBuffer.Get(), slots, m_pCamera->GetDirection());

    context->OMSetRenderTargets(1, &bloomRenderTarget, m_pDeviceResources->GetDepthStencil());
    context->OMSetDepthStencilState(m_pDeviceResources->GetTransDepthStencil(), 0);
    for (size_t i = 0; i < m_pModels.size(); ++i)
        m_pModels[i]->RenderTransparent(context, m_constantBufferData, m_pConstantBuffer.Get(), m_pMaterialBuffer.Get(), slots, m_pCamera->GetDirection(), true);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullsrv);
    context->OMSetRenderTargets(1, &renderTarget, m_pDeviceResources->GetDepthStencil());
}

void Renderer::Render()
{
    Clear();

    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();
    ID3D11RenderTargetView* renderTarget;

    D3D11_VIEWPORT viewport = m_pRenderTexture->GetViewPort();

    if (m_pSettings->GetShaderMode() == Settings::SETTINGS_PBR_SHADER_MODE::REGULAR)
    {
        RenderSimpleShadow();

        context->RSSetViewports(1, &viewport);

        renderTarget = m_pRenderTexture->GetRenderTargetView();
        context->OMSetRenderTargets(1, &renderTarget, m_pDeviceResources->GetDepthStencil());
        
        RenderEnvironment();
        RenderPlane();
        if (m_pSettings->GetSceneMode() == Settings::SETTINGS_SCENE_MODE::MODEL)
        {
            RenderModels();

            context->OMSetRenderTargets(0, nullptr, nullptr);
            m_pBloom->Process(context, m_pRenderTexture.get(), m_pDeviceResources->GetViewPort());
        }
        else
            RenderSphere(m_constantBufferData);
        
        PostProcessTexture();
    }
    else
    {
        context->RSSetViewports(1, &viewport);

        renderTarget = m_pDeviceResources->GetRenderTarget();
        context->OMSetRenderTargets(1, &renderTarget, m_pDeviceResources->GetDepthStencil());
        
        RenderSphere(m_constantBufferData);
    }
}

void Renderer::RenderSimpleShadow()
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

    context->OMSetRenderTargets(0, nullptr, m_pSimpleShadowMapDepthStencilView.Get());

    D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, static_cast<FLOAT>(simpleShadowMapSize), static_cast<FLOAT>(simpleShadowMapSize));

    context->RSSetViewports(1, &viewport);

    Model::ShadersSlots slots = { 3, 4, 5, 2, 0, 2 };

    WorldViewProjectionConstantBuffer cb;
    cb.Projection = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV2, 1, 0.1f, 10000.0f));

    DirectX::XMVECTOR lightPos = DirectX::XMLoadFloat4(&m_lightBufferData.LightPosition[0]);

    D3D11_BOX box = CD3D11_BOX(0, 0, 0, simpleShadowMapSize, simpleShadowMapSize, 1);
    context->RSSetState(m_pSimpleShadowMapRasterizerState.Get());
    for (UINT i = 0; i < 6; ++i)
    {
        cb.World = DirectX::XMMatrixIdentity();
        cb.View = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookToLH(lightPos, m_targers[i], m_ups[i]));

        context->ClearDepthStencilView(m_pSimpleShadowMapDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        if (m_pSettings->GetSceneMode() == Settings::SETTINGS_SCENE_MODE::MODEL)
        {

            for (size_t i = 0; i < m_pModels.size(); ++i)
                m_pModels[i]->Render(context, cb, m_pConstantBuffer.Get(), m_pMaterialBuffer.Get(), slots, false, false);

            for (size_t i = 0; i < m_pModels.size(); ++i)
                m_pModels[i]->RenderTransparent(context, cb, m_pConstantBuffer.Get(), m_pMaterialBuffer.Get(), slots, m_pCamera->GetDirection(), false, false);
        }
        else
            RenderSphere(cb, false);

        context->CopySubresourceRegion(m_pResourceSimpleShadowMapTexture.Get(), D3D11CalcSubresource(0, i, 1), 0, 0, 0, m_pDepthSimpleShadowMapTexture.Get(), 0, nullptr);
    }
    context->RSSetState(nullptr);
}

Renderer::~Renderer()
{}