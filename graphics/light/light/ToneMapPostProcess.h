#pragma once

#include "DeviceResources.h"

class ToneMapPostProcess
{
public:
    ToneMapPostProcess(const std::shared_ptr<DeviceResources>& deviceResources);
    ~ToneMapPostProcess();

    HRESULT CreateResources();

    void Process(ID3D11ShaderResourceView* sourceTexture);

private:
    std::shared_ptr<DeviceResources> m_pDeviceResources;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState;
};
