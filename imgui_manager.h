#ifndef IMGUI_MANAGER_H
#define IMGUI_MANAGER_H

#include <Windows.h>

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace ImGuiManager
{
    bool Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* context);
    void Finalize();

    void BeginFrame();
    void Render();            // 描画（Present前に呼ぶ）

    // WndProcから呼ぶ。ImGuiが入力を消費したら true を返す
    bool WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool IsVisible();
    void ToggleVisible();
}


#endif//IMGUI_MANAGER_H