#include "pch.h"

#include "ToneMapPostProcess.h"
#include "Utils.h"

ToneMapPostProcess::ToneMapPostProcess()
{};

HRESULT ToneMapPostProcess::CreateDeviceDependentResources(ID3D11Device* device)
{
    HRESULT hr = S_OK;

    std::vector<BYTE> bytes;

    // Create the vertex shader
    hr = CreateVertexShader(device, L"CopyVertexShader.cso", bytes, &m_pVertexShader);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = CreatePixelShader(device, L"ToneMapPixelShader.cso", bytes, &m_pPixelShader);
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
    if (FAILED(hr))
        return hr;

    m_pAverageLuminance = std::unique_ptr<AverageLuminanceProcess>(new AverageLuminanceProcess());
    hr = m_pAverageLuminance->CreateDeviceDependentResources(device);
    
    return hr;
}

HRESULT ToneMapPostProcess::CreateWindowSizeDependentResources(ID3D11Device* device, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    hr = m_pAverageLuminance->CreateWindowSizeDependentResources(device, width, height);

    return hr;
}

void ToneMapPostProcess::Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture, ID3D11RenderTargetView* renderTarget, D3D11_VIEWPORT viewport)
{
    m_pAverageLuminance->Process(context, sourceTexture);
    
    ID3D11ShaderResourceView* averageLuminance = m_pAverageLuminance->GetResultShaderResourceView();

    if (averageLuminance == nullptr)
        return;

    context->OMSetRenderTargets(1, &renderTarget, nullptr);
    context->RSSetViewports(1, &viewport);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, &sourceTexture);
    context->PSSetShaderResources(1, 1, &averageLuminance);
    context->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
    
    context->Draw(3, 0);
}

ToneMapPostProcess::~ToneMapPostProcess()
{}
