/*#include<Windows.h>
#include<sstream>
#include"debug_ostream.h"

constexpr char FILE_NAME[] = "tekito.png";

int APIENTRY WinMain(_In_ HINSTANCE hIstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    std::stringstream ss;
    ss << "テクスチャファイル" << FILE_NAME << "が読み込めませんでした";//std::coutと同じ
  int a= MessageBox(nullptr,ss.str().c_str(), "キャプション", MB_YESNOCANCEL|MB_ICONERROR| MB_DEFBUTTON2);//ユーザーへのメッセージウィンドウ
            //MB_DEFBUTTON2はキャプションのデフォルトボタン印を２つめのボタンにつける（ユーザーに選んで欲しい所につける）
  if (a == IDYES) {
      MessageBox(nullptr, "YES", "YES", MB_OK);
  }
  else if (a == IDCANCEL) {
      MessageBox(nullptr, "キャンセル", "キャンセル", MB_OK);
  }
    return 0;
}*/


#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include"Game_Window.h"
#include"direct3d.h"
#include"polygon.h"
#include"texture.h"
#include"sprite.h"
#include"sprite_anim.h"
#include"collision.h"
#include"fade.h"
#include"debug_text.h"
#include<sstream>
#include"system_timer.h"
#include<math.h>
#include"key_logger.h"
#include"mouse.h"
#include<Xinput.h>
#include"Audio.h"
#include"scene.h"
#include"game.h"
#include"shader3d.h"
#include"cube_.h"
#include"grid.h"
#include"meshfield.h"
#include"shader2d.h"
#include"sampler.h"
#include"light.h"
#include"shader3d_unlit.h"
#include"shader_depth.h"
#include "imgui_manager.h"
#include"editor_ui.h"
#include"player.h"
#include "gamepad.h"


#pragma comment(lib,"xinput.lib")



/*-----------------------------------------
    メイン
-----------------------------------------*/
int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    (void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    /*WindowsのCOM（Component Object Model）を初期化しています。
DirectXやWIC（画像読み込み系）など、COMベースのAPIを使う前にこれが必要。*/

    //DPIスケーリング
    /*PCのディスプレイ設定の拡大率は１２５％推奨で、コンソール画面も拡大されるようにするやつ*/
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HWND hWnd = GameWindow_Create(hInstance);
    /*GameWindow_Create関数で ウィンドウを作成して、ウィンドウハンドル（HWND）を返す自作関数
    Game_Window.cppに設定ある*/  

    /*direct.cpp初期化、各ファイルでも初期化
    それぞれの初期化関数の中身を順に追っていくことで、描画までの流れが完全に理解できるようになります。*/
    SystemTimer_Initialize();
    KeyLogger_Initialize();
    Mouse_Initialize(hWnd);
    InitAudio();
    

    Direct3D_Initialize(hWnd);
    //Polygon_Initialize(Direct3D_GetDevice(),Direct3D_GetContext());
    Sampler_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Texture_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Shader2D_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Shader3D_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Shader3DUnlit_Initialize();
    ShaderDepth_Initialize();
    Sprite_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    SpriteAnim_Initialize();
    Cube_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    //Grid_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    MeshField_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    Light_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
    ImGuiManager::Initialize(hWnd, Direct3D_GetDevice(), Direct3D_GetContext());

   

    
#if defined(DEBUG)||defined(_DEBUG)//リリースモードのときデバック用collisionを商品化で見えないようにする
    hal::DebugText dt(Direct3D_GetDevice(), Direct3D_GetContext(), L"consolab_ascii_512.png",
        Direct3D_GetBackBufferWidth(), Direct3D_GetBackBufferHeight(),
        0.0f, 0.0f,//ここからデバッグテキストの左上の位置を決める
        0, 0,
        0.0f, 16.0f);

    Collision_DebugInitialize(Direct3D_GetDevice(), Direct3D_GetContext());
#endif//ここまで灰色なる

    //テクスチャの管理番号を代入(CPUが識別するためのID）
    /*他の関数（たとえば Sprite_Draw など描画系の関数）は、この Texture_Load 関数が返す
    「管理番号（インデックス）」を使って、プロジェクトに読み込まれたテクスチャにアクセスします。
     なぜ管理番号で管理するの？
無駄な重複読み込みを防ぐ（メモリ節約）

テクスチャへのアクセスが高速

コードがシンプルになる（ポインタ渡しより安全）
*/
    
    

   //{}は一様初期化
    //Mouse_SetVisible(false); //マウスのカーソル見えなくする

     Scene_Initialize();

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    //fps,実行フレーム計測用(ハードによって処理速度変わるのを無くすため）
    double exec_last_time = SystemTimer_GetTime();
    double fps_last_time = exec_last_time;
    double current_time = 0.0;
    ULONG frame_count = 0;
    double fps = 0.0;


    /* ゲームループ＆メッセージループ */
    MSG msg;


    do {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) { // ウィンドウメッセージが来ていたら 
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else { // 来てなかったらゲームの処理 

            //fps計測
            // fps計測
            current_time = SystemTimer_GetTime(); // システム時刻を取得
            double elapsed_time = current_time - fps_last_time; // fps計測用の経過時間を計算



            if (elapsed_time >= 1.0) // 1秒ごとに計測
            {
                fps = frame_count / elapsed_time;
                fps_last_time = current_time; // FPSを測定した時刻を保存
                frame_count = 0; // カウントをクリア
            }
            /*double time = SystemTimer_GetTime();
            double elapsed_time = time - register_time;

            if (elapsed_time < 1.0 / 60.0)continue;

           if (elapsed_time >= 1.0) {
                fps = frame_count / elapsed_time;
                frame_count = 0;
                register_time = time;
           };*/


           

           
           //1/60秒ごとに実行
            elapsed_time = current_time - exec_last_time;
            if (elapsed_time >= (1.0 / 60.0)) {
                //if(true){  //ifどっちか
                exec_last_time = current_time;//処理した時刻を保存


                //ゲームの更新
                KeyLogger_Update();
                Gamepad_Update();
                Mouse_State ms{};
                Mouse_GetState(&ms);
                Scene_Update(elapsed_time);
                SpriteAnim_Update(elapsed_time);
                Game_Update(elapsed_time);

#if defined(DEBUG)||defined(_DEBUG)
                // Update前後どっちでもOK
                ImGuiManager::BeginFrame();

                // ここでUIを構築
                if (ImGuiManager::IsVisible())
                {
                    EditorUI_Draw(elapsed_time);
                }
                else
                {
                    // UI閉じたら上書き解除（安全）
                    Player_SetInputOverride(false, nullptr);
                }
#endif
                //ここからゲームの描画
                Direct3D_SetBackBuffer();
                Direct3D_ClearBackBuffer();
                Game_Draw();//3D
               
                // ImGui描画（Present前）
                ImGuiManager::Render();

                

#if defined(DEBUG)||defined(_DEBUG)
                std::stringstream ss;//coutと同じようにできるようにするやつ
                ss << "count:" << frame_count << std::endl;
                dt.SetText(ss.str().c_str());

               //dt.SetText("ABCDE\n");//文字の途中で\n入れるとエラーなる　末尾のみ
               // dt.SetText("FG\n", { 0.0f,1.0f,1.0f,1.0f });

              // dt.Draw();
               //dt.Clear();
#endif

                Direct3D_Present();

                Scene_Refresh();

                frame_count++;
            }
        }

                 
        
    } while (msg.message != WM_QUIT);

#if defined(DEBUG)||defined(_DEBUG)
    Collision_DebugFinalize();
#endif

    ImGuiManager::Finalize();

    Light_Finalize();

    Cube_Finalize();
    Grid_Finalize();
    MeshField_Finalize();

    Scene_Finalize();

    Shader2D_Finalize();
    Shader3D_Finalize();
    Shader3DUnlit_Finalize();
    ShaderDepth_Finalize();
    Sampler_Finalize();
    //Polygon_Finalize();

    Sprite_Finalize();

    Direct3D_Finalize();

    Texture_Finalize();

    SpriteAnim_Finalize();

    //Fade_Finalize();

    Mouse_Finalize();

    UninitAudio();

    return (int)msg.wParam;
}


