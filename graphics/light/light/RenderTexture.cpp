#include "pch.h"

#include "RenderTexture.h"

RenderTexture::RenderTexture(const std::shared_ptr<DeviceResources>& deviceResources) :
	m_pDeviceResources(deviceResources),
    m_format(DXGI_FORMAT_R16G16B16A16_FLOAT)
{};

HRESULT RenderTexture::CreateResources()
{
    HRESULT hr = S_OK;

    ID3D11Device* device = m_pDeviceResources->GetDevice();

    // Create a render target
    CD3D11_TEXTURE2D_DESC rtd(
        m_format,
        m_pDeviceResources->GetWidth(),
        m_pDeviceResources->GetHeight(),
        1,
        1,
        D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        D3D11_USAGE_DEFAULT,
        0,
        1
    );

    hr = device->CreateTexture2D(&rtd, nullptr, m_pRenderTarget.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create render target view
    CD3D11_RENDER_TARGET_VIEW_DESC rtvd(D3D11_RTV_DIMENSION_TEXTURE2D, m_format);

    hr = device->CreateRenderTargetView(m_pRenderTarget.Get(), &rtvd, m_pRenderTargetView.ReleaseAndGetAddressOf());
    if (FAILED(hr))
        return hr;

    // Create shader resource view
    CD3D11_SHADER_RESOURCE_VIEW_DESC srvd(D3D11_SRV_DIMENSION_TEXTURE2D, m_format);

    hr = device->CreateShaderResourceView(m_pRenderTarget.Get(), &srvd, m_pShaderResourceView.ReleaseAndGetAddressOf());

    return hr;
}

RenderTexture::~RenderTexture()
{}
