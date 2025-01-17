#pragma once

#include "DeviceResources.h"
#include "RenderTexture.h"
#include "ShaderStructures.h"
#include "ToneMapPostProcess.h"
#include "Camera.h"

class Renderer
{
public:
    Renderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<Camera>& camera);
    ~Renderer();

    HRESULT CreateDeviceDependentResources();
    HRESULT CreateWindowSizeDependentResources();
    
    HRESULT OnResize();

    void UpdateLightColor(UINT index, float factor);

    void Update();
    void Render();

private:
    HRESULT CreateShaders();
    HRESULT CreateRectangle();
    HRESULT CreateTexture();
    HRESULT CreateLights();

    void UpdatePerspective();

    void Clear();
    void RenderInTexture();
    void PostProcessTexture();

    std::shared_ptr<DeviceResources>      m_pDeviceResources;
    std::unique_ptr<RenderTexture>        m_pRenderTexture;
    std::unique_ptr<ToneMapPostProcess>   m_pToneMap;
    std::shared_ptr<Camera>               m_pCamera;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>        m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pLightPositionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pLightColorBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pTexture;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>       m_pSamplerLinear;

    WorldViewProjectionConstantBuffer m_constantBufferData;
    LightPositionConstantBuffer       m_lightPositionBufferData;
    LightColorConstantBuffer          m_lightColorBufferData;
    
    UINT32 m_indexCount;
    UINT32 m_frameCount;
};
