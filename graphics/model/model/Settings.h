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

    enum class SceneMode
    {
        MODEL = 0,
        SPHERES
    };

    Settings(const std::shared_ptr<DeviceResources>& deviceResources);
    ~Settings();

    void CreateResources(HWND hWnd);

    PBRShaderMode GetShaderMode() const { return m_shaderMode; };
    SceneMode GetSceneMode() const { return m_sceneMode; };
    float GetLightStrength(UINT index) const;

    DirectX::XMFLOAT4 GetAlbedo() const { return DirectX::XMFLOAT4(m_albedo); };

    void Render();

private:
    std::shared_ptr<DeviceResources> m_pDeviceResources;

    PBRShaderMode m_shaderMode;
    SceneMode     m_sceneMode;

    float m_strengths[NUM_LIGHTS];
    float m_albedo[4] = { 0.25f, 1.0f, 1.0f, 1.0f };
};
