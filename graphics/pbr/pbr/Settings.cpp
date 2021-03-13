#include "pch.h"

#include "Settings.h"
#include "../../ImGui/imgui_impl_dx11.h"
#include "../../ImGui/imgui_impl_win32.h"

Settings::Settings(const std::shared_ptr<DeviceResources>& deviceResources):
    m_pDeviceResources(deviceResources),
    m_shaderMode(PBRShaderMode::REGULAR)
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

    ImGui::Begin("Settings");

    static const char* shaderModes[] = { "Regular", "Normal distribution", "Geometry", "Fresnel" };
    ImGui::Combo("Shader mode", reinterpret_cast<int*>(&m_shaderMode), shaderModes, IM_ARRAYSIZE(shaderModes));

    ImGui::End();

    ImGui::Render();
    
    ID3D11RenderTargetView* renderTarget = m_pDeviceResources->GetRenderTarget();
    ID3D11DepthStencilView* depthStencil = m_pDeviceResources->GetDepthStencil();
    m_pDeviceResources->GetDeviceContext()->OMSetRenderTargets(1, &renderTarget, depthStencil);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

Settings::~Settings()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
