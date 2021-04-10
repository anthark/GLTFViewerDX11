#include "pch.h"

#include "Settings.h"
#include "../../ImGui/imgui_impl_dx11.h"
#include "../../ImGui/imgui_impl_win32.h"

Settings::Settings(const std::shared_ptr<DeviceResources>& deviceResources) :
    m_pDeviceResources(deviceResources),
    m_shaderMode(SETTINGS_PBR_SHADER_MODE::REGULAR),
    m_sceneMode(SETTINGS_SCENE_MODE::MODEL),
    m_strengths()
{};

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
    ImGui::SetNextWindowSize(ImVec2(500, 175), ImGuiCond_Once);
    ImGui::Begin("Settings");

    static const char* shaderModes[] = { "Regular", "Normal distribution", "Geometry", "Fresnel" };
    ImGui::Combo("Shader mode", reinterpret_cast<int*>(&m_shaderMode), shaderModes, IM_ARRAYSIZE(shaderModes));

    static const char* sceneModes[] = { "Model", "Spheres" };
    ImGui::Combo("Scene content", reinterpret_cast<int*>(&m_sceneMode), sceneModes, IM_ARRAYSIZE(sceneModes));

    ImGui::ColorEdit3("Albedo", m_albedo);

    for (size_t i = 0; i < NUM_LIGHTS; ++i)
        ImGui::SliderFloat((std::string("Strength of ") + std::to_string(i) + std::string(" light")).c_str(), m_strengths + i, 0.0f, 500.0f);

    ImGui::End();

    ImGui::Render();
    
    ID3D11RenderTargetView* renderTarget = m_pDeviceResources->GetRenderTarget();
    ID3D11DepthStencilView* depthStencil = m_pDeviceResources->GetDepthStencil();
    m_pDeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &renderTarget, depthStencil);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

float Settings::GetLightStrength(UINT index) const
{
    if (index >= NUM_LIGHTS)
        return 0.0f;
    return m_strengths[index];
}

Settings::~Settings()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
