#include "imgui_manager.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool g_visible = false;   // 起動時はUI非表示（F1で出す）

bool ImGuiManager::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();

    // キーボード操作で完結したいならON（Tab移動や矢印で操作できる）
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // （任意）ドッキングしたいなら
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplWin32_Init(hwnd)) return false;
    if (!ImGui_ImplDX11_Init(device, context)) return false;

    return true;
}

void ImGuiManager::Finalize()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiManager::BeginFrame()
{
    if (!g_visible) return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::Render()
{
    if (!g_visible) return;

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiManager::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!g_visible) return false;
    return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

bool ImGuiManager::IsVisible() { return g_visible; }
void ImGuiManager::ToggleVisible() { g_visible = !g_visible; }

