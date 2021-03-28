#pragma once

#include "DeviceResources.h"
#include "RenderTexture.h"
#include "ShaderStructures.h"
#include "ToneMapPostProcess.h"
#include "Camera.h"
#include "Settings.h"

class Renderer
{
public:
    Renderer(const std::shared_ptr<DeviceResources>& deviceResources, const std::shared_ptr<Camera>& camera, const std::shared_ptr<Settings>& settings);
    ~Renderer();

    HRESULT CreateDeviceDependentResources();
    HRESULT CreateWindowSizeDependentResources();
    
    HRESULT OnResize();

    void Update();
    void Render();

private:
    HRESULT CreateShaders();
    HRESULT CreateSphere();
    HRESULT CreateLights();
    HRESULT CreateTexture();
    HRESULT CreateCubeTexture();
    HRESULT CreateIrradianceTexture();
    HRESULT CreateCubeTextureFromResource(UINT size, ID3D11Texture2D* dst, ID3D11ShaderResourceView* src, ID3D11VertexShader* vs, ID3D11PixelShader* ps);

    void UpdatePerspective();

    void Clear();
    void RenderInTexture();
    void RenderEnvironment();
    void PostProcessTexture();

    std::unique_ptr<RenderTexture>        m_pRenderTexture;
    std::unique_ptr<ToneMapPostProcess>   m_pToneMap;
    std::shared_ptr<Camera>               m_pCamera;
    std::shared_ptr<DeviceResources>      m_pDeviceResources;
    std::shared_ptr<Settings>             m_pSettings;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>        m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>        m_pIrradianceInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pEnvironmentVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pEnvironmentTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pEnvironmentShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pEnvironmentCubeTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pEnvironmentCubeShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pIrradianceTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pIrradianceShaderResourceView;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>       m_pSamplerLinear;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pEnvironmentVertexShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pIrradianceVertexShader;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>       m_pEnvironmentCubeVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pNDPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pGPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pFPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pEnvironmentPixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pIrradiancePixelShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        m_pEnvironmentCubePixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pLightPositionBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pLightColorBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>             m_pMaterialBuffer;

    WorldViewProjectionConstantBuffer m_constantBufferData;
    LightPositionConstantBuffer       m_lightPositionBufferData;
    LightColorConstantBuffer          m_lightColorBufferData;
    MaterialConstantBuffer            m_materialBufferData;
    
    UINT32 m_indexCount;
    UINT32 m_frameCount;

    DirectX::XMVECTOR m_targers[6] = {
        DirectX::XMVectorSet(1, 0, 0, 0),
        DirectX::XMVectorSet(-1, 0, 0, 0),
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, -1, 0, 0),
        DirectX::XMVectorSet(0, 0, 1, 0),
        DirectX::XMVectorSet(0, 0, -1, 0)
    };

    DirectX::XMVECTOR m_ups[6] = {
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, 0, -1, 0),
        DirectX::XMVectorSet(0, 0, 1, 0),
        DirectX::XMVectorSet(0, 1, 0, 0),
        DirectX::XMVectorSet(0, 1, 0, 0)
    };

    DirectX::XMFLOAT3 m_squareLeftBottomAngles[6] = {
        DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),
        DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f),
        DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f),
        DirectX::XMFLOAT3(-0.5f, -0.5f, -0.5f),
        DirectX::XMFLOAT3(-0.5f, -0.5f, 0.5f),
        DirectX::XMFLOAT3(0.5f, -0.5f, -0.5f)
    };

    DirectX::XMFLOAT3 m_squareRightTopAngles[6] = {
        DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f),
        DirectX::XMFLOAT3(-0.5f, 0.5f, 0.5f),
        DirectX::XMFLOAT3(0.5f, 0.5f, -0.5f),
        DirectX::XMFLOAT3(0.5f, -0.5f, 0.5f),
        DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f),
        DirectX::XMFLOAT3(-0.5f, 0.5f, -0.5f)
    };
};
