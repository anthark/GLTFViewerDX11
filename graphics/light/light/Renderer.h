#pragma once

#include "DeviceResources.h"
#include "ShaderStructures.h"

class Renderer
{
public:
    Renderer(const std::shared_ptr<DeviceResources>& deviceResources);
    ~Renderer();

    HRESULT CreateDeviceDependentResources();
    void    CreateWindowSizeDependentResources();
    void    UpdatePerspective();
    void    Update();
    void    Render();

private:
    HRESULT ReadCompiledShader(const WCHAR* szFileName, BYTE** bytes, size_t& bufferSize);
    HRESULT CreateShaders();
    HRESULT CreateTriangle();

    std::shared_ptr<DeviceResources> m_pDeviceResources;

    Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_pIndexBuffer;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_pVertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_pPixelShader;
    Microsoft::WRL::ComPtr<ID3D11Buffer>        m_pConstantBuffer;

    WorldViewProjectionConstantBuffer m_constantBufferData;
    
    UINT32 m_indexCount;
    UINT32 m_frameCount;
};
