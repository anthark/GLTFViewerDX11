#include "pch.h"

#include "ToneMapPostProcess.h"
#include "Utils.h"

ToneMapPostProcess::ToneMapPostProcess()
{};

HRESULT ToneMapPostProcess::CreateResources(ID3D11Device* device)
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
    
    return hr;
}

void ToneMapPostProcess::Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture)
{
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
    context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
    context->PSSetShaderResources(0, 1, &sourceTexture);
    context->PSSetSamplers(0, 1, m_pSamplerState.GetAddressOf());
    
    context->Draw(3, 0);
}

ToneMapPostProcess::~ToneMapPostProcess()
{}
