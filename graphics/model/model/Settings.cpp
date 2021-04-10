#include "pch.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "Settings.h"
#include "../../ImGui/imgui_impl_dx11.h"
#include "../../ImGui/imgui_impl_win32.h"

Settings::Settings(const std::shared_ptr<DeviceResources>& deviceResources) :
    m_pDeviceResources(deviceResources),
    m_shaderMode(SETTINGS_PBR_SHADER_MODE::REGULAR),
    m_sceneMode(SETTINGS_SCENE_MODE::MODEL),
    m_lightsStrengths(),
    m_lightsThetaAngles(),
    m_lightsPhiAngles(),
    m_lightsDistances(),
    m_lightsColors()
{
    for (UINT i = 0; i < NUM_LIGHTS; ++i)
        m_lightsColors[i][0] = m_lightsColors[i][1] = m_lightsColors[i][2] = 1.0f;
};

void Settings::CreateResources(HWND hWnd)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(m_pDeviceResources->GetDevice(), m_pDeviceResources->GetDeviceContext());
    ImGui::StyleColorsDark();
}

void Settings::Render()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(500, 125), ImGuiCond_Once);

    ImGui::Begin("Settings");

    static const char* shaderModes[] = { "Regular", "Normal distribution", "Geometry", "Fresnel" };
    ImGui::Combo("Shader mode", reinterpret_cast<int*>(&m_shaderMode), shaderModes, IM_ARRAYSIZE(shaderModes));

    static const char* sceneModes[] = { "Model", "Spheres" };
    ImGui::Combo("Scene content", reinterpret_cast<int*>(&m_sceneMode), sceneModes, IM_ARRAYSIZE(sceneModes));

    ImGui::ColorEdit3("Albedo", m_albedo);

    ImGui::End();

    for (UINT i = 0; i < NUM_LIGHTS; ++i)
    {
        ImGui::SetNextWindowPos(ImVec2(0, 125 + 175 * static_cast<float>(i)), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(500, 175), ImGuiCond_Once);

        ImGui::Begin((std::string("Light ") + std::to_string(i)).c_str());

        ImGui::SliderFloat("Distance", m_lightsDistances + i, 10.0f, 1000.0f);

        ImGui::SliderFloat("Theta", m_lightsThetaAngles + i, 0.0f, static_cast<float>(M_PI));

        ImGui::SliderFloat("Phi", m_lightsPhiAngles + i, 0.0f, static_cast<float>(2 * M_PI));

        ImGui::ColorEdit3("Color", m_lightsColors[i]);

        ImGui::SliderFloat("Strength", m_lightsStrengths + i, 0.0f, 500.0f);

        ImGui::End();
    }

    ImGui::Render();
    
    ID3D11RenderTargetView* renderTarget = m_pDeviceResources->GetRenderTarget();
    ID3D11DepthStencilView* depthStencil = m_pDeviceResources->GetDepthStencil();
    m_pDeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &renderTarget, depthStencil);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

DirectX::XMFLOAT4 Settings::GetLightColor(UINT index) const
{
    if (index >= NUM_LIGHTS)
        return DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    float color[4] = {};
    for (UINT i = 0; i < 3; ++i)
        color[i] = m_lightsColors[index][i];
    color[3] = m_lightsStrengths[index];
    return DirectX::XMFLOAT4(color);
}

DirectX::XMFLOAT4 Settings::GetLightPosition(UINT index) const
{
    if (index >= NUM_LIGHTS)
        return DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    float position[4] = {};
    position[0] = m_lightsDistances[index] * static_cast<float>(sin(m_lightsThetaAngles[index])) * static_cast<float>(cos(m_lightsPhiAngles[index]));
    position[2] = m_lightsDistances[index] * static_cast<float>(sin(m_lightsThetaAngles[index])) * static_cast<float>(sin(m_lightsPhiAngles[index]));
    position[1] = m_lightsDistances[index] * static_cast<float>(cos(m_lightsThetaAngles[index]));
    position[3] = 0.0f;
    return DirectX::XMFLOAT4(position);
}

Settings::~Settings()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
