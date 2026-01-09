/*==================================================================================
ゲームウィンドウ[GameWindow.cpp]



----------------------------------------------------------------------------------------
=========================================================================================*/
#include "Game_Window.h"
#include"keyboard.h"
#include"mouse.h"
#include "system_timer.h"
#include "imgui_manager.h" 
#include "imgui.h"
#include "imgui_impl_win32.h"
#include<algorithm>
#include <Windows.h>


/*-----------------------------------------
    ウィンドウプロシージャ プロトタイプ宣言
-----------------------------------------*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*-----------------------------------------
    ウィンドウ情報
-----------------------------------------*/
static constexpr char WINDOW_CLASS[] = "GameWindow";  // メインウィンドウクラス名
static constexpr char TITLE[] = "ウィンドウ表示";      // タイトルバーのテキスト






HWND GameWindow_Create(HINSTANCE hInstance)
{
    /* ウィンドウクラスの登録 
    どんなウィンドウにするか決めてる公式に色々クラスある*/
    WNDCLASSEX wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr; // メニューは作らない
    wcex.lpszClassName = WINDOW_CLASS;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    RegisterClassEx(&wcex);

    /* メインウィンドウの作成 */
    constexpr int SCREEN_WIDTH = 1600;
    constexpr int SCREEN_HEIGHT = 900;

    RECT window_rect{ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

    DWORD style = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);

    AdjustWindowRect(&window_rect, style, FALSE);

    const int WINDOW_WIDTH = window_rect.right - window_rect.left;
    const int WINDOW_HEIGHT = window_rect.bottom - window_rect.top;



    // デスクトップのサイズを取得
    // ※プライマリモニターの画面解像度取得
    int desktop_width = GetSystemMetrics(SM_CXSCREEN);
    int desktop_height = GetSystemMetrics(SM_CYSCREEN);

    // ウィンドウの表示位置をデスクトップの真ん中に調整する
    const int WINDOW_X = std::max((desktop_width - WINDOW_WIDTH) / 2, 0);
    const int WINDOW_Y = std::max((desktop_height - WINDOW_HEIGHT) / 2, 0);

    HWND hWnd = CreateWindow(
        WINDOW_CLASS,
        TITLE,
        style,
        WINDOW_X,
        WINDOW_Y,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );
    return hWnd;
}


/*-------------------------------------------------
ウィンドウプロシージャ
-------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#if defined(DEBUG)||defined(_DEBUG)
    // --- F1：ImGui 表示/非表示 ＋ マウスモード切り替え ---
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && wParam == VK_F1)
    {
        ImGuiManager::ToggleVisible();

        if (ImGuiManager::IsVisible())
        {
            // UI操作：カーソル表示＆ロック解除
            Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);
            Mouse_SetVisible(true);
        }
        else
        {
            // ゲーム操作：カーソル非表示＆相対入力（ロック）
            Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
            // Relative側で ShowCursor(FALSE) & ClipCursor() されるので SetVisible は不要
        }

        // SetModeで立てたイベントを「今このメッセージで」反映させる（キーでもOK）
        Mouse_ProcessMessage(message, wParam, lParam);

        return 0;
    }

    if(ImGuiManager::WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }
#endif

    switch (message)
    {
     case WM_ACTIVATE:
        if (wParam != WA_INACTIVE) {
            SystemTimer_Start();
        }
        break;
     case WM_ACTIVATEAPP:
         Keyboard_ProcessMessage(message, wParam, lParam);
         Mouse_ProcessMessage(message, wParam, lParam);
         SystemTimer_Stop();
         break;
     case WM_INPUT:
     case WM_MOUSEMOVE:
     case WM_LBUTTONDOWN:
     case WM_LBUTTONUP:
     case WM_RBUTTONDOWN:
     case WM_RBUTTONUP:
     case WM_MBUTTONDOWN:
     case WM_MBUTTONUP:
     case WM_MOUSEWHEEL:
     case WM_XBUTTONDOWN:
     case WM_XBUTTONUP:
     case WM_MOUSEHOVER:
         Mouse_ProcessMessage(message, wParam, lParam);
         break;
         case WM_KEYDOWN:
             if (wParam == VK_ESCAPE) {
                 SendMessage(hWnd, WM_CLOSE, 0, 0);
             }
         case WM_SYSKEYDOWN:
         case WM_KEYUP:
         case WM_SYSKEYUP:
             Keyboard_ProcessMessage(message, wParam, lParam);
             break;
    case WM_DESTROY: // ウィンドウの破棄メッセージ
        PostQuitMessage(0); // WM_QUITメッセージの送信
        break;
    default:
        // 通常のメッセージ処理はこの関数に任せる
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}