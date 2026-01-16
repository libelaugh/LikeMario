/*==============================================================================

　　  ステージmagma管理[stage_magma_manager.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/
#include "stage_magma_manager.h"
#include"stage_magma_make.h"
#include"direct3d.h"
#include"texture.h"
#include"camera.h"
#include"player_camera.h"
#include"key_logger.h"
#include"sampler.h"
#include"meshfield.h"
#include"light.h"
#include"model.h"
#include"player.h"
#include "gamepad.h"
//#include"cube_.h"
//#include"map.h"
//#include"billboard.h"
//#include "shader_billboard.h"
#include"sprite_anim.h"
#include"mouse.h"
#include"sprite.h"
#include "shader_depth.h"
#include"map_camera.h"
#include"light_camera.h"
#include"stage01_manage.h"
#include "imgui_manager.h"
#include "imgui.h"
#include"item.h"
#include"sky.h"
#include"goal.h"
#include"Audio.h"
#include <type_traits>
#include <utility>
#include <cmath>
#include<DirectXMath.h>

using namespace DirectX;

static int magmaBgm = -1;

static int testTex = -1;
static int g_animId = -1;
static int g_animPlayId = -1;

static bool g_isDebug = false;

static bool g_padDrivingPlayer = false;

static DirectX::XMFLOAT3 g_spawnPos = { 0,5,-9 };
static DirectX::XMFLOAT3 g_spawnFront = {};
static const char* g_stageJsonPath = "stage_magma.json";

static constexpr float kMagmaBaseY = -7.0f;
static float meshFieldPosY = kMagmaBaseY;

static GoalState g_goalMagma= GoalState::Active;

static void StageMagmaManager_SetStageInfo(const StageInfo& info)
{
	g_spawnPos = info.spawnPos;
	g_spawnFront = info.spawnFront;
	g_stageJsonPath = info.jsonPath;
}

namespace {
	DirectX::XMFLOAT3 g_CubePos = { 0.0f, 0.0f, 0.0f };
	float  g_x = 0.0f, g_y = 0.0f, g_z = 0.0f;
	float  g_angle = 0.0f, g_scale = 1.0f;
	double g_AccumulatedTime = 0.0;

	static XMFLOAT3 g_cubePos{};//
	static XMFLOAT3 g_cubeVel{};
}

namespace
{
	template<class T, class = void> struct has_moveX : std::false_type {};
	template<class T> struct has_moveX<T, std::void_t<decltype(std::declval<T&>().moveX)>> : std::true_type {};
	template<class T, class = void> struct has_moveY : std::false_type {};
	template<class T> struct has_moveY<T, std::void_t<decltype(std::declval<T&>().moveY)>> : std::true_type {};
	template<class T, class = void> struct has_jump : std::false_type {};
	template<class T> struct has_jump<T, std::void_t<decltype(std::declval<T&>().jump)>> : std::true_type {};
	template<class T, class = void> struct has_dash : std::false_type {};
	template<class T> struct has_dash<T, std::void_t<decltype(std::declval<T&>().dash)>> : std::true_type {};
	template<class T, class = void> struct has_spin : std::false_type {};
	template<class T> struct has_spin<T, std::void_t<decltype(std::declval<T&>().spin)>> : std::true_type {};
	template<class T, class = void> struct has_crouch : std::false_type {};
	template<class T> struct has_crouch<T, std::void_t<decltype(std::declval<T&>().crouch)>> : std::true_type {};

	inline void SetMoveX(PlayerInput& in, float v) { if constexpr (has_moveX<PlayerInput>::value) in.moveX = v; }
	inline void SetMoveY(PlayerInput& in, float v) { if constexpr (has_moveY<PlayerInput>::value) in.moveY = v; }
	inline void SetJump(PlayerInput& in, bool  v) { if constexpr (has_jump<PlayerInput>::value)  in.jump = v; }
	inline void SetDash(PlayerInput& in, bool  v) { if constexpr (has_dash<PlayerInput>::value)  in.dash = v; }
	inline void SetSpin(PlayerInput& in, bool  v) { if constexpr (has_spin<PlayerInput>::value)  in.spin = v; }
	inline void SetCrouch(PlayerInput& in, bool v) { if constexpr (has_crouch<PlayerInput>::value) in.crouch = v; }
}



static void mapRendering() {
	// レンダーターゲットをテクスチャへ
	Direct3D_SetOffscreen();
	Direct3D_ClearOffscreen();

	// ライトカメラ（行列）の設定
	XMFLOAT4X4 mtxView = LightCamera_GetViewMatrix();
	XMFLOAT4X4 mtxProj = LightCamera_GetProjectionMatrix();
	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = XMLoadFloat4x4(&mtxProj);

	// カメラに関する行列をシェーダーに設定する
	Camera_SetMatrix(view, proj);

	// テクスチャーサンプラーの設定
	Sampler_SetFilterAnisotropic();

	// マップ用ライト
// ※ディレクショナルライトの色を真っ黒にしてしまって、アンビエントライトのみにするか
//  ディレクショナルライトを真下に向けるか、ライティングはゲームそのままにするか
	Light_SetAmbient({ 1.0f, 1.0f, 1.0f });
	Light_SetDirectionalWorld({ 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f, 0.0f });
	//Light_SetLimLight({ 0.0f, 0.0f, 0.0f }, 0.0f);

	// 深度有効
	Direct3D_SetDepthEnable(true);


	//Enemy_Draw();
	Player_Draw();
	//Map_Draw();
}

static void lightRendering() {
	// レンダーターゲットをテクスチャへ
	Direct3D_SetShadowDepth();
	Direct3D_ClearShadowDepth();

	// ライトカメラ（行列）の設定
	XMFLOAT4X4 mtxView = LightCamera_GetViewMatrix();
	XMFLOAT4X4 mtxProj = LightCamera_GetProjectionMatrix();
	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = XMLoadFloat4x4(&mtxProj);

	// カメラに関する行列をシェーダーに設定する
	Camera_SetMatrix(view, proj);

	ShaderDepth_SetViewMatrix(view);
	ShaderDepth_SetProjectionMatrix(proj);

	// 深度有効
	Direct3D_SetDepthEnable(true);

	//キャスト(影を落とすオブジェクト)
	//Enemy_DepthDraw();
	Player_DepthDraw();
	//Map_Draw();
}

DirectX::XMFLOAT3 StageMagmaManager_GetSpawnPosition()
{
	return g_spawnPos;
}

const char* StageMagmaManager_GetStageJsonPath()
{
	return g_stageJsonPath;
}

float StageMagmaManager_GetMagmaY()
{
	return meshFieldPosY;
}

void StageMagmaManager_AddMagmaY(float a)
{
	meshFieldPosY += a;
}

void StageMagmaManager_SetMagmaY(float y){
	meshFieldPosY = y;
	}

float StageMagmaManager_GetMagmaBaseY()
 {
	return kMagmaBaseY;
	}


void StageMagmaManager_Initialize(const StageInfo& info)
{
	StageMagmaManager_SetStageInfo(info);
	Player_Initialize(g_spawnPos, g_spawnFront); //({ 6.5f, 3.0f, 1.0f }, { 0,0,1 });
	Camera_Initialize({ 0.004,4.8,-8.7 }, { 0, -0.5, 0.85 }, { 0,0.85,0.53 }, { 1,0,0 });
	PlayerCamera_Initialize();
	//Map_Initialize();
	//Enemy_Initialize();
	//Bullet_Initialize();
	Sky_Initialize();
	//Billboard_Initialize();
	//BulletHitEffect_Initialize();
	testTex = Texture_Load(L"runningman001.png");
	g_animId = SpriteAnim_RegisterPattern(testTex, 10, 5, 0.1, { 140,200 }, { 0,0, });
	LightCamera_Initialize({ -1.0f,-1.0f,1.0f }, { 0.0f,20.0f,-0.0f });

	g_animPlayId = SpriteAnim_CreatePlayer(g_animId);

	//Enemy_Create({ 1.0f,0.0f,1.0f });
	//Enemy_Create({ 1.0f,5.0f,1.0f });

	g_isDebug = false;

	Stage01_Initialize(g_stageJsonPath);
	Goal_Init();
	Goal_SetPosition({ 6.0f, 22.0f, 42.0f });

	magmaBgm = LoadAudio("magma.wav");
	PlayAudio(magmaBgm, true);

}

void StageMagmaManager_ChangeStage(const StageInfo& info)
{
	StageMagmaManager_SetStageInfo(info);
	StageMagma_SetPlayerPositionAndLoadJson(g_spawnPos, g_stageJsonPath);
}

void StageMagmaManager_Finalize()
{
	UnloadAudio(magmaBgm);
	Goal_Uninit();
	//BulletHitEffect_Finalize();
		//Enemy_Finalize();
		//Map_Finalize();
		//Bullet_Finalize();
	Sky_Finalize();
	Camera_Finalize();
	//Billboard_Finalize();



	Stage01_Finalize();
	Item_Finalize();
}

void StageMagmaManager_Update(double elapsedTime)
{
	//Camera_UpdateKeys(elapsed_time);
	//Camera_UpdateInput();
	//float aspect = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	//Camera_UpdateMatrices(aspect);

	//WndProcでホイール拾う
	//case WM_MOUSEWHEEL: Camera_OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)); break;

	// Gamepad input (XInput)
	{
		const GoalState gs = Goal_GetState();
		if (gs == GoalState::Clear || gs == GoalState::WaitInput)
		{
			// パッドでPlayer overrideしてたら解除（これしないと入力が残る）
			if (g_padDrivingPlayer)
			{
				Player_SetInputOverride(false, nullptr);
				g_padDrivingPlayer = false;
			}

			Goal_Update(elapsedTime);
			g_goalMagma = Goal_GetState();
			return;
		}
	}

	GamepadState pad{};
	bool ok = Gamepad_GetState(0, &pad);

	/*static float logT = 0.0f;
	logT += elapsedTime;
	if (logT > 0.5f)
	{
		logT = 0.0f;
		char buf[256];
		sprintf_s(buf, "pad ok=%d lx=%.3f ly=%.3f  a=%d b=%d x=%d y=%d\n",
			ok ? 1 : 0, pad.lx, pad.ly,
			pad.a ? 1 : 0, pad.b ? 1 : 0, pad.x ? 1 : 0, pad.y ? 1 : 0);
		OutputDebugStringA(buf);
	}*/
	bool padConnected = ok && pad.connected;

	if (padConnected)
	{
		const float PAD_MOVE_EPS = 0.15f;
		const bool padHasStickInput = (std::abs(pad.lx) > PAD_MOVE_EPS) || (std::abs(pad.ly) > PAD_MOVE_EPS);
		const bool padHasTriggerInput = (pad.lt > 0.1f) || (pad.rt > 0.1f);
		const bool padHasButtonInput = pad.a || pad.b || pad.x || pad.y || pad.start || pad.back || pad.lb || pad.rb;
		const bool padActive = padHasStickInput || padHasTriggerInput || padHasButtonInput;

		// Editor/MotionLab が既に override 中なら邪魔しない。
		// 自分が前フレームから握ってる場合だけ更新する。
		if (padActive && (!Player_IsInputOverrideEnabled() || g_padDrivingPlayer))
		{
			PlayerInput in{};
			SetMoveX(in, pad.lx);
			SetMoveY(in, pad.ly);

			//まず移動だけ（ボタンは割り当て確定してから）
			SetJump(in, pad.a);
			SetSpin(in, pad.x);
			SetCrouch(in, pad.lt);


			Player_SetInputOverride(true, &in);
			g_padDrivingPlayer = true;
		}
		else if (!padActive && g_padDrivingPlayer)
		{
			Player_SetInputOverride(false, nullptr);
			g_padDrivingPlayer = false;
		}
	}
	else
	{
		// パッドが切れたら override を戻す
		if (g_padDrivingPlayer)
		{
			Player_SetInputOverride(false, nullptr);
			g_padDrivingPlayer = false;
		}
	}


	SpriteAnim_Update(elapsedTime);





	StageMagma_Update(elapsedTime);
	Stage01_Update(elapsedTime);

	Player_Update(elapsedTime);
	PlayerCamera_Update(elapsedTime);

	Item_Update();

	Goal_Update(elapsedTime);
	g_goalMagma = Goal_GetState();

	if (g_goalMagma == GoalState::Clear)
	{
		StopAudio(magmaBgm);
	}
}

void StageMagmaManager_Draw()
{
	if (g_goalMagma == GoalState::Clear || g_goalMagma == GoalState::WaitInput)
	{
		Goal_DrawUI();
		return;
	}
	mapRendering();
	lightRendering();

	Direct3D_SetBackBuffer();

	Direct3D_SetDepthShadowTexture(2);

	//レンダーターゲットをバックバッフアへ
	Direct3D_SetOffscreen();
	Direct3D_ClearOffscreen();
	Direct3D_SetDepthEnable(true);
	Direct3D_SetBackBuffer();

	static int draw_count = 0;
	Mouse_State ms;
	Mouse_GetState(&ms);

	XMFLOAT4X4 mtxView = g_isDebug ? Camera_GetMatrix() : PlayerCamera_GetViewMatrix();
	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = g_isDebug ? XMLoadFloat4x4(&Camera_GetPerspectiveMatrix()) : XMLoadFloat4x4(&PlayerCamera_GetPerspectiveMatrix());
	XMFLOAT3 camera_position = g_isDebug ? Camera_GetPosition() : PlayerCamera_GetPosition();

	Sky_SetPosition(camera_position);

	auto lv = LightCamera_GetViewMatrix();
	auto lp = LightCamera_GetProjectionMatrix();

	Direct3D_SetLightViewProjectionMatrix(
		XMLoadFloat4x4(&lv) * XMLoadFloat4x4(&lp)
	);
	/*Direct3D_SetLightViewProjectionMatrix(
		XMLoadFloat4x4(&LightCamera_GetViewMatrix()) * XMLoadFloat4x4(&LightCamera_GetProjectionMatrix()));
		*/

	XMFLOAT3 test = Direct3D_ScreenToWorld(ms.x, ms.y, 0.0f, mtxView, PlayerCamera_GetPerspectiveMatrix());

	if (draw_count % 2 != 0) {
		XMFLOAT3 position = Player_GetPosition();
		position.y = 100.0f;

		MapCamera_SetPosition(position);
		MapCamera_SetFront(Player_GetFront());
		mtxView = MapCamera_GetViewMatrix();
		XMFLOAT4X4 mtxProj = MapCamera_GetProjectionMatrix();
		view = XMLoadFloat4x4(&mtxView);
		proj = XMLoadFloat4x4(&mtxProj);
	}

	//カメラに関する行列をシェーダに設定する
	Camera_SetMatrix(view, proj);

	//
	//ShaderBillboard_SetViewMatrix(view);
	//ShaderBillboard_SetProjectionMatrix(proj);
	//Billboard_SetViewMatrix(mtxView);

	//テクスチャサンプラーの設定
	Sampler_SetFilterAnisotropic();

	//空の表示
	Direct3D_SetDepthDepthWriteDisable();
	Sky_Draw();
	Direct3D_SetDepthEnable(true);

	//各種ライトの設定
	float ambientColor = 0.8f;
	Light_SetAmbient({ ambientColor,ambientColor,ambientColor });
	Light_SetDirectionalWorld({ -0.7f,-0.7f,0.7f,0.0f }, { 0.3f,0.25f,0.3f,1.0f });//世界の平行光
	//Light_SetDirectionalWorld({ -0.7f,-0.7f,0.7f,0.0f }, { 0.0f,0.0f,0.0f,1.0f });//世界の平行光

	Light_SetPointLightCount(3);
	XMMATRIX rot = XMMatrixRotationY(g_angle);
	XMVECTOR position = XMVector3Transform({ 0.0f,0.3f,-3.0f }, rot);
	XMFLOAT3 pp; XMStoreFloat3(&pp, position);
	//Light_SetPointLight(0, pp, 6.0f, { 1.0f,0.0f,0.0f });
	//Light_SetPointLight(1, { -2.0f,2.0f,0.0f }, 6.0f, { 0.0f,1.0f,0.0f });
	//Light_SetPointLight(2, { 2.0f,2.0f,2.0f }, 6.0f, { 0.0f,0.0f,1.0f });


	//Grid_Draw();

	XMMATRIX mtxWorld =
		XMMatrixRotationY(g_angle) *
		XMMatrixTranslationFromVector(XMLoadFloat3(&g_cubePos));
	//Cube_Draw(mtxWorld);

	//Cube_Draw(mtxWorld);

	// 回転・拡縮・平行移動すべて無し → 単位行列
	XMMATRIX W = XMMatrixIdentity();//Identitiy単位行列作る関数
	XMMATRIX W1 = W; W1 = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	//Cube_Draw(W1);

	XMMATRIX W2 = XMMatrixIdentity();
	float w2_offset = MeshField_GetHalf(); 
	W2 = XMMatrixTranslation(-w2_offset, meshFieldPosY, -w2_offset+45.0f);
	Direct3D_SetDepthShadowTexture(2);
	MeshField_Draw(W2);

	/*Sampler_SetFilterAnisotropic();
	XMMATRIX theWorld = XMMatrixTranslation(3.0f, 0.5f, 2.0f);
	Cube_Draw(theWorld);*/

	//Cube_Draw02(2, XMMatrixTranslation(test.x,test.y,test.z ));


	//Enemy_Draw();
	Player_Draw();
	Goal_Draw3D();
	//Map_Draw();

	//Bullet_Draw();

	//Billboard_Draw(testTex, { -10.0f,0.0f,-10.0f }, { 3.5f,5.0f }, { 140 * 3,140,200 }, { 0.0f,-2.5f });
	//BillboardAnim_Draw(g_animPlayId, { -10.0f,0.0f,-10.0f }, { 3.5f,5.0f }, { 0.0f,-2.5f });


	if (g_isDebug) {
		Camera_DebugDraw();
	}





	Camera_SetMatrix(view, proj);//必要
	Stage01_Draw();
	Item_Draw();

}

