/*==============================================================================

	ゲーム本体[game.h]
														 Author : Tanaka Kouki
														 Date   : 2026/01/01
--------------------------------------------------------------------------------

==============================================================================*/

//ゲームを全て関数で動くようにする
#include "game.h"
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
#include <type_traits>
#include <utility>
#include<DirectXMath.h>

using namespace DirectX;


static MODEL* g_pModelKarby = nullptr;
static MODEL* g_pModelWoodenBarrel = nullptr;
static MODEL* g_pModelSword1 = nullptr;

static int testTex = -1;
static int g_animId = -1;
static int g_animPlayId = -1;

static bool g_isDebug = false;

static bool g_padDrivingPlayer = false;

static const char* g_startStageJson = "lab_action.json";

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


void mapRendering();
void lightRendering();

void Game_Initialize()
{
	g_pModelKarby = ModelLoad("model/karby.fbx",0.1,false);
	g_pModelWoodenBarrel = ModelLoad("model/Wooden_Barrel/Wooden_Barrel.fbx", 0.01, false);
	g_pModelSword1 = ModelLoad("model/Sting-Sword lowpoly.fbx", 0.1, false);

	Player_Initialize({ 0.0f, 3.0f, 0.0f }, { 0,0,1 }); //({ 6.5f, 3.0f, 1.0f }, { 0,0,1 });
	Camera_Initialize({ 0.004,4.8,-8.7 }, { 0, -0.5, 0.85 }, { 0,0.85,0.53 }, { 1,0,0 });
	PlayerCamera_Initialize();
	//Map_Initialize();
	//Enemy_Initialize();
	//Bullet_Initialize();
	//Sky_Initialize();
	//Billboard_Initialize();
	//BulletHitEffect_Initialize();
	testTex = Texture_Load(L"runningman001.png");
	g_animId = SpriteAnim_RegisterPattern(testTex, 10, 5, 0.1, { 140,200 }, { 0,0, });
	LightCamera_Initialize({ -1.0f,-1.0f,1.0f }, { 0.0f,20.0f,-0.0f });

	g_animPlayId = SpriteAnim_CreatePlayer(g_animId);

	//Enemy_Create({ 1.0f,0.0f,1.0f });
	//Enemy_Create({ 1.0f,5.0f,1.0f });

	g_isDebug = false;



	Stage01_Initialize(g_startStageJson);

}

void Game_Finalize()
{
	//BulletHitEffect_Finalize();
	//Enemy_Finalize();
	ModelRelease(g_pModelKarby);
	ModelRelease(g_pModelWoodenBarrel);
	ModelRelease(g_pModelSword1);
	//Map_Finalize();
	//Bullet_Finalize();
	//Sky_Finalize();
	Camera_Finalize();
	//Billboard_Finalize();



	Stage01_Finalize();

}

void Game_Update(float elapsedTime)
{

	//Camera_UpdateKeys(elapsed_time);
	//Camera_UpdateInput();
	//float aspect = (float)Direct3D_GetBackBufferWidth() / Direct3D_GetBackBufferHeight();
	//Camera_UpdateMatrices(aspect);

	//WndProcでホイール拾う
	//case WM_MOUSEWHEEL: Camera_OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam)); break;

	// Gamepad input (XInput)

	GamepadState pad{};
	bool ok = Gamepad_GetState(0, &pad);

	static float logT = 0.0f;
	logT += elapsedTime;
	if (logT > 0.5f)
	{
		logT = 0.0f;
		char buf[256];
		sprintf_s(buf, "pad ok=%d lx=%.3f ly=%.3f\n", ok ? 1 : 0, pad.lx, pad.ly);
		OutputDebugStringA(buf);
	}
	bool padConnected = Gamepad_GetState(0, &pad) && pad.connected;

	if (padConnected)
	{
		// Editor/MotionLab が既に override 中なら邪魔しない。
		// 自分が前フレームから握ってる場合だけ更新する。
		if (!Player_IsInputOverrideEnabled() || g_padDrivingPlayer)
		{
			auto ApplyDeadzone = [](float v)
				{
					return (fabsf(v) < 0.20f) ? 0.0f : v; //0.15〜0.25で調整
				};

			pad.lx = ApplyDeadzone(pad.lx);
			pad.ly = ApplyDeadzone(pad.ly);

			PlayerInput in{};
			SetMoveX(in, pad.lx);
			SetMoveY(in, pad.ly);

			//まず移動だけ（ボタンは割り当て確定してから）
			// SetJump(in, pad.a);
			// SetDash(in, (pad.rt > 0.35f) || pad.rb);
			// SetSpin(in, pad.x || pad.lb);
			// SetCrouch(in, pad.b || (pad.lt > 0.35f));


			Player_SetInputOverride(true, &in);
			g_padDrivingPlayer = true;
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

	Player_Update(elapsedTime);
	//Enemy_Update(elapsedTime);
	//Sky_SetPosition(PlayerCamera_GetPosition());

	if (KeyLogger_IsTrigger(KK_L)) {
		g_isDebug = !g_isDebug;
	}
	if (g_isDebug) {
		Camera_Update(elapsedTime); //キー対応のみのカメラ
	}
	else {
		PlayerCamera_Update(elapsedTime);
	}

	//Bullet_Update(elapsedTime);

	//弾とマップとの当たり判定（AABB vs AABB)
	/*for (int j = 0; j < Map_GetObjectsCount(); j++) {
		for (int i = 0; i < Bullet_GetBulletsCount(); i++) {
			AABB bullet = Bullet_GetAABB(i);
			AABB object = Map_GetObject(j)->aabb;
			if (Collision_IsOverlapAABB(bullet, object)) {
				BulletHitEffect_Create(Bullet_GetPosition(i));
				Bullet_Destroy(i);
			}
		}
	}*/

	//敵と弾との当たり判定
	/*for (int j = 0; j < Enemy_GetEnemyCount(); j++) {
		for (int i = 0; i < Bullet_GetBulletsCount(); i++) {

			Sphere bullet = Bullet_GetSphere(i);
			Sphere enemy = Enemy_GetEnemy(j)->GetCollision();

			if (Collision_IsOverlapSphere(bullet, enemy)) {
				BulletHitEffect_Create(Bullet_GetPosition(i));
				Bullet_Destroy(i);
				Enemy_GetEnemy(j)->Damage(50);
			}
		}
	}*/

	SpriteAnim_Update(elapsedTime);
	//BulletHitEffect_Update();//引数にelapsedTime無くてもUpdateは呼ばれる

	//Cube_Update(elapsed_time);
	g_AccumulatedTime += elapsedTime;

	//g_x = sin(g_AccumulatedTime) * 5.0f;

	//g_y = sin(g_AccumulatedTime) * 1.0f;
	//g_z = -sin(g_AccumulatedTime) * 1.0f;
	//g_y = sin(g_AccumulatedTime)* 1.5f;
	g_angle = (float) - g_AccumulatedTime * 2.0f;
	//g_scale = (sin(g_AccumulatedTime)+1)*0.5f * 5.0f;
	g_scale = 1.0f;

	/*if (KeyLogger_IsTrigger(KK_SPACE)) {
		g_cubePos = Camera_GetPosition();
		XMStoreFloat3(&g_cubeVel, XMLoadFloat3(&Camera_GetFront()) * 10.0f);
	}*/

	// 毎フレームの更新は「保存した速度」を使う（カメラに依存させない）
	XMVECTOR cube_position = XMLoadFloat3(&g_cubePos);
	cube_position += XMLoadFloat3(&g_cubeVel) * elapsedTime;
	XMStoreFloat3(&g_cubePos, cube_position);






	Stage01_Update(elapsedTime);
	/*ステージ切り替えはこんな感じ
	const char* path = "stage02.json";

StageSwitchResult r = Stage01_SwitchStage(path, true);

if (r == STAGE_SWITCH_CREATED_EMPTY)
{
    // ここで「新規ステージ（未保存）です」みたいな表示を出しても良い
}
*/
}

void Game_Draw()
{
	ID3D11DeviceContext* ctx = Direct3D_GetContext();
	float bf[4] = {};
	ctx->OMSetBlendState(nullptr, bf, 0xffffffff); // ← 不透明(ブレンドOFF)

	mapRendering();
	lightRendering();

	Direct3D_SetBackBuffer();

	Direct3D_SetDepthShadowTexture(2);

	//レンダーターゲットをバックバッフアへ
	Direct3D_SetOffscreen();
	Direct3D_SetBackBuffer();

	static int draw_count = 0;
	Mouse_State ms;
	Mouse_GetState(&ms);

	XMFLOAT4X4 mtxView = g_isDebug ? Camera_GetMatrix() : PlayerCamera_GetViewMatrix();
	XMMATRIX view = XMLoadFloat4x4(&mtxView);
	XMMATRIX proj = g_isDebug ? XMLoadFloat4x4(&Camera_GetPerspectiveMatrix()) : XMLoadFloat4x4(&PlayerCamera_GetPerspectiveMatrix());
	XMFLOAT3 camera_position = g_isDebug ? Camera_GetPosition() : PlayerCamera_GetPosition();

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
	//Direct3D_SetDepthDepthWriteDisable();
	//Sky_Draw();
	//Direct3D_SetDepthEnable(true);

	//各種ライトの設定
	float ambientColor = 0.8f;
	Light_SetAmbient({ ambientColor,ambientColor,ambientColor});
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
	float w2_offset = MeshField_GetHalf(); W2 = XMMatrixTranslation(-w2_offset, 0, -w2_offset);
	Direct3D_SetDepthShadowTexture(2);
	MeshField_Draw(W2);

	/*Sampler_SetFilterAnisotropic();
	XMMATRIX theWorld = XMMatrixTranslation(3.0f, 0.5f, 2.0f);
	Cube_Draw(theWorld);*/

	//Cube_Draw02(2, XMMatrixTranslation(test.x,test.y,test.z ));
	
	Light_SetSpecularWorld(PlayerCamera_GetPosition(), 30.0f, { 0.2f,0.2f,0.2f,1.0f });
	//ModelDraw(g_pModelKarby, XMMatrixTranslation(-1,0,0));

	Light_SetSpecularWorld(PlayerCamera_GetPosition(), 50.0f, { 0.1f,0.1f,0.1f,1.0f });
	//ModelDraw(g_pModelWoodenBarrel, XMMatrixTranslation(-3, 0, 0));

	ModelDraw(g_pModelSword1, XMMatrixTranslation(-5, 0, 0));

	//Enemy_Draw();
	Player_Draw();
	
	//Map_Draw();

	//Bullet_Draw();

	//Billboard_Draw(testTex, { -10.0f,0.0f,-10.0f }, { 3.5f,5.0f }, { 140 * 3,140,200 }, { 0.0f,-2.5f });
	//BillboardAnim_Draw(g_animPlayId, { -10.0f,0.0f,-10.0f }, { 3.5f,5.0f }, { 0.0f,-2.5f });

	//BulletHitEffect_Draw();

	/*XMFLOAT3 cube_pos;
	XMStoreFloat3(&cube_pos, vtest);
	XMFLOAT2 pos = Direct3D_WorldToScreen(cube_pos, mtxView, PlayerCamera_GetPosition());
	Sprite_Draw(g_TestTex01, pos.x, pos.y, 128, 128);*/

	/*　＝＝＝＝ミニマップ表示だが修正が必要＝＝＝＝
	Direct3D_SetDepthEnable(false);
	if (draw_count % 2 != 0) {
		Direct3D_SetOffscreenTexture(0);
		Sprite_Draw(0, 0, 256, 256);
	}
	draw_count++;*/

	//ピラミッド   処理落ちする=========MineCraftはなぜ処理落ちしないのか良い研究なる==================
	/*const XMMATRIX mtrRotation = XMMatrixRotationY(g_angle);
	for (int j = 0; j < 3; j++) {
		for (int k = 0; k < 5 - j - j; k++) {
			for (int i = 0; i < 5 - j - j; i++) {
				g_x = -2.0f;
				g_z = -2.0f;
				XMMATRIX mtrTlanslation = XMMatrixTranslation(g_x + i * 1 + j, g_y + j * 1 + 0.5f, g_z + k * 1 + j);
				XMMATRIX mtrScale = XMMatrixScaling(g_scale, g_scale, g_scale);

				//全体回転
				XMMATRIX mtrWorld = mtrScale * mtrTlanslation * mtrRotation;

				Cube_Draw(mtrWorld);
			}
		}
	}*/


	if (g_isDebug) {
		Camera_DebugDraw();
	}





	Camera_SetMatrix(view, proj);//必要
	Stage01_Draw();

}

void mapRendering() {
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

void lightRendering() {
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