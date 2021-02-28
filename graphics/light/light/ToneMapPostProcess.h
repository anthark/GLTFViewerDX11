#pragma once

#include "DeviceResources.h"

class ToneMapPostProcess
{
public:
    ToneMapPostProcess();
    ~ToneMapPostProcess();

    HRESULT CreateResources(ID3D11Device* device);

    void Process(ID3D11DeviceContext* context, ID3D11ShaderResourceView* sourceTexture);

private:
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  m_pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> m_pSamplerState;
};
