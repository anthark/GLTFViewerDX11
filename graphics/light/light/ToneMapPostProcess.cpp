#include "pch.h"

#include "ToneMapPostProcess.h"
#include "Utils.h"

ToneMapPostProcess::ToneMapPostProcess(const std::shared_ptr<DeviceResources>& deviceResources) :
	m_pDeviceResources(deviceResources)
{};

HRESULT ToneMapPostProcess::CreateResources()
{
    HRESULT hr = S_OK;

    BYTE* bytes = nullptr;
    size_t bufferSize;

    ID3D11Device* device = m_pDeviceResources->GetDevice();

    // Read the vertex shader
    hr = ReadCompiledShader(L"PostProcessVertexShader.cso", &bytes, bufferSize);
    if (FAILED(hr))
        return hr;

    // Create the vertex shader
    hr = device->CreateVertexShader(bytes, bufferSize, nullptr, &m_pVertexShader);
    delete[] bytes;
    if (FAILED(hr))
        return hr;

    // Read the pixel shader
    hr = ReadCompiledShader(L"PostProcessPixelShader.cso", &bytes, bufferSize);
    if (FAILED(hr))
        return hr;

    // Create the pixel shader
    hr = device->CreatePixelShader(bytes, bufferSize, nullptr, &m_pPixelShader);
    delete[] bytes;
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

void ToneMapPostProcess::Process(ID3D11ShaderResourceView* sourceTexture)
{
    ID3D11DeviceContext* context = m_pDeviceResources->GetDeviceContext();

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
