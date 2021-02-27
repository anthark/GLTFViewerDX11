#pragma once

#include "DeviceResources.h"

class RenderTexture
{
public:
    RenderTexture(const std::shared_ptr<DeviceResources>& deviceResources);
    ~RenderTexture();

    HRESULT CreateResources();

    ID3D11RenderTargetView*   GetRenderTargetView() const   { return m_pRenderTargetView.Get(); };
    ID3D11ShaderResourceView* GetShaderResourceView() const { return m_pShaderResourceView.Get(); };

private:
    std::shared_ptr<DeviceResources> m_pDeviceResources;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pRenderTarget;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>   m_pRenderTargetView;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pShaderResourceView;

    DXGI_FORMAT m_format;
};
