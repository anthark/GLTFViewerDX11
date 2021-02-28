#include "pch.h"

#include "AverageLuminanceProcess.h"
#include "Utils.h"

AverageLuminanceProcess::AverageLuminanceProcess()
{}

ID3D11ShaderResourceView* AverageLuminanceProcess::GetResultShaderResourceView() const
{ 
    if (m_renderTextures.size() == 0)
        return nullptr;

    return m_renderTextures[m_renderTextures.size() - 1].GetShaderResourceView(); 
}

HRESULT AverageLuminanceProcess::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    std::vector<BYTE> bytes;

    // Create the vertex shader
    hr = CreateVertexShader(device, L"CopyVertexShader.cso", bytes, &m_pVertexShader);
    if (FAILED(hr))
        return hr;

    // Create the copy pixel shader
    hr = CreatePixelShader(device, L"CopyPixelShader.cso", bytes, &m_pCopyPixelShader);
    if (FAILED(hr))
        return hr;

    // Create the luminance pixel shader
    hr = CreatePixelShader(device, L"LuminancePixelShader.cso", bytes, &m_pLuminancePixelShader);
    if (FAILED(hr))
        return hr;

    // Create the sampler state
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sd.MaxAnisotropy = D3D11_MAX_MAXANISOTROPY;
    hr = device->CreateSamplerState(&sd, m_pSamplerState.GetAddressOf());

    return hr;
}

HRESULT AverageLuminanceProcess::CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    size_t minSize = static_cast<size_t>(min(width, height));
    size_t n;
    for (n = 0; static_cast<size_t>(1) << (n + 1) <= minSize; n++);
    m_renderTextures.clear();
    m_renderTextures.reserve(n + 2);

    UINT size = 1 << n;
    RenderTexture initTexture(DXGI_FORMAT_R16G16B16A16_FLOAT);
    hr = initTexture.CreateResources(device, size, size);
    if (FAILED(hr))
        return hr;
    m_renderTextures.push_back(initTexture);
    
    for (size_t i = 0; i <= n; i++)
    {
        size = 1 << (n - i);
        RenderTexture texture(DXGI_FORMAT_R16G16B16A16_FLOAT);
        hr = texture.CreateResources(device, size, size);
        if (FAILED(hr))
            return hr;
        m_renderTextures.push_back(texture);
    }

    return hr;
}

void AverageLuminanceProcess::CopyTexture(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture, RenderTexture& dst, ID3D11PixelShader* pixelShader)
{
    ID3D11RenderTargetView* renderTarget = dst.GetRenderTargetView();

    D3D11_VIEWPORT viewport = dst.GetViewPort();

    context->OMSetRenderTargets(1, &renderTarget, nullptr);
    context->RSSetViewports(1, &viewport);

    context->PSSetShader(pixelShader, nullptr, 0);
    context->PSSetShaderResources(0, 1, &sourceTexture);
    
    context->Draw(3, 0);
}

void AverageLuminanceProcess::Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture)
{
    if (m_renderTextures.size() == 0)
        return;

    float backgroundColour[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    for (size_t i = 0; i < m_renderTextures.size(); i++)
        context->ClearRenderTargetView(m_renderTextures[i].GetRenderTargetView(), backgroundColour);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);

    context->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());

    CopyTexture(context, sourceTexture, m_renderTextures[0], m_pCopyPixelShader.Get());
    CopyTexture(context, m_renderTextures[0].GetShaderResourceView(), m_renderTextures[1], m_pLuminancePixelShader.Get());

    for (size_t i = 2; i < m_renderTextures.size(); i++)
    {
        CopyTexture(context, m_renderTextures[i - 1].GetShaderResourceView(), m_renderTextures[i], m_pCopyPixelShader.Get());
    }
}

AverageLuminanceProcess::~AverageLuminanceProcess()
{}
