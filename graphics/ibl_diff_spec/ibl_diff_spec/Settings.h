#pragma once

#include "DeviceResources.h"
#include "ShaderStructures.h"
#include "../../ImGui/imgui.h"

class Settings
{
public:
    enum class PBRShaderMode
    {
        REGULAR = 0,
        NORMAL_DISTRIBUTION,
        GEOMETRY,
        FRESNEL
    };

    enum class MetalType
    {
        ALUMINIUM = 0,
        ARGENTUM,
        AURUM,
        CUPRUM,
        FERRUM
    };

    Settings(const std::shared_ptr<DeviceResources>& deviceResources);
    ~Settings();

    void CreateResources(HWND hWnd);

    PBRShaderMode GetShaderMode() const { return m_shaderMode; };
    float GetLightStrength(UINT index) const;
    
    DirectX::XMFLOAT3 GetMetalF0() const { return m_metalF0[static_cast<UINT>(m_metalType)]; };

    DirectX::XMFLOAT3 GetAlbedo() const { return DirectX::XMFLOAT3(m_albedo); };

    void Render();

private:
    std::shared_ptr<DeviceResources> m_pDeviceResources;

    PBRShaderMode m_shaderMode;
    MetalType     m_metalType;

    float m_strengths[NUM_LIGHTS];
    float m_albedo[3] = { 0.25f, 1.0f, 1.0f };

    DirectX::XMFLOAT3 m_metalF0[5] = {
        DirectX::XMFLOAT3(0.91f, 0.92f, 0.92f),
        DirectX::XMFLOAT3(0.95f, 0.93f, 0.88f),
        DirectX::XMFLOAT3(1.00f, 0.71f, 0.29f),
        DirectX::XMFLOAT3(0.95f, 0.64f, 0.54f),
        DirectX::XMFLOAT3(0.56f, 0.57f, 0.58f)
    };
};
