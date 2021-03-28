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

    Settings(const std::shared_ptr<DeviceResources>& deviceResources);
    ~Settings();

    void CreateResources(HWND hWnd);

    PBRShaderMode GetShaderMode() const { return m_shaderMode; };
    float GetLightStrength(UINT index) const;

    void Render();

private:
    std::shared_ptr<DeviceResources> m_pDeviceResources;

    PBRShaderMode m_shaderMode;

    float m_strengths[NUM_LIGHTS];
};
