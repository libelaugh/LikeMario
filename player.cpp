/*==============================================================================

	プレイヤー制御[player.h]
														 Author : Kouki Tanaka
														 Date   : 2025/10/31
--------------------------------------------------------------------------------

==============================================================================*/

#include "player.h"
#include"model.h"
#include"key_logger.h"
#include"light.h"
#include"camera.h"
#include"player_camera.h"
//#include"cube_.h"
//#include"map.h"
#include "model_skinned_fixed.h"
//#include "stage01_manage.h"
// #include "stage_map.h"  
#include"collision.h"
#include "debug_ostream.h"
#include "imgui_manager.h"
#include "gamepad.h"
#include "player_action.h"
#include"billboard.h"
#include "stage_simple_manager.h"
#include<DirectXMath.h>
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <cfloat>

using namespace DirectX;

static void DumpLogAABB(const char* name, const AABB& a)
{
	//※ログ出力はかなり重いから多用注意
	const float sx = a.max.x - a.min.x;
	const float sy = a.max.y - a.min.y;
	const float sz = a.max.z - a.min.z;

	hal::dout
		<< "[AABB] " << name
		<< " min(" << a.min.x << "," << a.min.y << "," << a.min.z << ")"
		<< " max(" << a.max.x << "," << a.max.y << "," << a.max.z << ")"
		<< " size(" << sx << "," << sy << "," << sz << ")"
		<< std::endl;
}
static inline float ClampFloat(float v, float lo, float hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}
//サイズ０にして描画とコリジョンを消す
static void HideStageBlockRuntime(int index)
{
	StageBlock* block = Stage01_GetMutable(index);
	if (!block) return;

	block->positionOffset.y -= 10000.0f;
	block->sizeOffset.x = -block->size.x;
	block->sizeOffset.y = -block->size.y;
	block->sizeOffset.z = -block->size.z;
	Stage01_RebuildObject(index);
}

// ちょい下を調べて「床がある」なら groundY(床の上面Y) を返す
static bool ProbeGroundY(const DirectX::XMVECTOR& position, float eps, float* outGroundY)
{
	if (outGroundY) *outGroundY = 0.0f;

	AABB playerAabb = Player_ConvertPositionToAABB(position);

	bool found = false;
	float bestY = -FLT_MAX;

	const int n = Stage01_GetCount();
	for (int i = 0; i < n; ++i)
	{
		const StageBlock* b = Stage01_Get(i);
		if (!b) continue;

		const AABB& box = b->aabb;

		// XZ の足裏が乗ってるか（投影でチェック）
		const bool overlapXZ = !(playerAabb.max.x <= box.min.x || playerAabb.min.x >= box.max.x ||
			playerAabb.max.z <= box.min.z || playerAabb.min.z >= box.max.z);
		if (!overlapXZ) continue;

		// 足元が床の上面からどれだけ上か
		const float dy = playerAabb.min.y - box.max.y;

		// dyが 0..eps（＋微小な誤差は許容）なら接地扱い
		if (dy >= -0.002f && dy <= eps)
		{
			if (box.max.y > bestY)
			{
				bestY = box.max.y;
				found = true;
			}
		}
	}

	if (found && outGroundY) *outGroundY = bestY;
	return found;
}


static XMFLOAT3 g_playerPos{};
static XMFLOAT3 g_playerFront{0.0f,0.0f,1.0f};
static XMFLOAT3 g_playerVel{};

static bool g_isGrounded = false;

//static MODEL* g_playerModel{ nullptr };
static SKINNED_MODEL* g_playerModel{ nullptr };


static constexpr float PLAYER_SCALE = 14.0f;//14.0fに決定
static constexpr float PLAYER_DRAW_Y_OFFSET = 0.0f;//1.0f * PLAYER_SCALE;

static constexpr float PLAYER_HALF_WIDTH = 0.25f;  // 左右の半分(AABBだから軸に平行、OBBならプレイヤーのローカル座標系)
static constexpr float PLAYER_HALF_DEPTH = 0.25f;  // 前後の半分
static constexpr float PLAYER_HEIGHT = 0.9f;  // 足元から頭まで

static const PlayerTuning k_defaultTune{
	18.0f,           // jumpImpulse
	10.0f,     // gravity
	-7.0f,           // terminalFall
	1000.0f / 40.0f, // moveAccel加速度
	10.0f,           // friction
	DirectX::XM_2PI * 1.0f // rotSpeed
};

static PlayerTuning g_tune = k_defaultTune;

PlayerTuning* Player_GetTuning() { return &g_tune; }
const PlayerTuning& Player_GetDefaultTuning() { return k_defaultTune; }
void Player_ResetTuning() { g_tune = k_defaultTune; }


static bool g_inputOverride = false;
static PlayerInput g_overrideInput{};


// ===== Action FSM =====
static PlayerActionState  g_act{};
static PlayerActionParams g_actParam{};

static float g_brakeTimer = 0.0f;     // seconds remaining
static float g_dash2AccelDist = 0.0f;// distance traveled during dash2 input (for 2-stage accel)

// ===== Variable jump (3-step by A hold time) =====
static bool  g_varJumpActive = false;      // true while ascending after a ground jump
static float g_varJumpHoldT = 0.0f;        // seconds A is held since jump start (clamped)
static float g_varJumpStartSpeedY = 0.0f;  // initial Y speed for this jump ("strong")
static bool  g_varJumpCutApplied = false;  // cut applied once on early release

// ===== Visual-only draw offset (cancel animation forward slide) =====
// ===== Visual-only fixed draw offset (jump/land) =====
static bool  g_visFixJump = false;
static bool  g_visFixLand = false;
static bool  g_visFixCrouchForwardJump2 = false;

// 調整値（ワールド単位）: 前にズレるなら「マイナス」で後ろに引く
static float g_visJumpForwardFix = -3.0f;
static float g_visLandForwardFix = -1.0f;
static float g_visCrouchForwardJump2Fix = -0.5f;

// ===== Landing-timed 2nd jump (着地2段ジャンプ) =====
static bool  s_airFromGroundJump = false;   // 通常の地上ジャンプ後、空中にいる間 true
static float s_doubleJumpWindowT = 0.0f;    // 着地後の2段ジャンプ受付残り時間
static bool  s_prevJumpHeldInput = false;   // 入力のエッジ検出用
static float s_jumpBufferT = 0.0f;          // 着地タイミング用の入力バッファ

static float g_spinYaw = 0.0f; // 見た目だけ回転

// ===== Crouch forward jump =====
static bool  g_crouchForwardJumpActive = false;
static XMFLOAT3 g_crouchForwardJumpDir = { 0.0f, 0.0f, 1.0f };
static float s_crouchFJumpMoveLockT = 0.0f;


void Player_SetInputOverride(bool enable, const PlayerInput* input)
{
	g_inputOverride = enable;
	if (input) g_overrideInput = *input;
	else g_overrideInput = PlayerInput{};
}

bool Player_IsInputOverrideEnabled()
{
	return g_inputOverride;
}


void Player_DebugTeleport(const DirectX::XMFLOAT3& pos, bool resetVelocity)
{
	g_playerPos = pos;
	if (resetVelocity)
		g_playerVel = { 0,0,0 };
}

bool Player_IsGrounded()
{
	return g_isGrounded;
}


void Player_Initialize(const XMFLOAT3& position, const XMFLOAT3& front)
{
	g_playerPos = position;
	g_playerVel = { 0.0f,0.0f,0.0f };
    XMStoreFloat3(&g_playerFront, XMVector3Normalize(XMLoadFloat3(&front)));

	//g_playerModel = ModelLoad("model/atlas/scene.gltf", 0.2f, false);
	g_playerModel = SkinnedModel_Load("model/atlas/scene.gltf", 1.0f, false);

	PlayerAction_Init(g_act);
	PlayerAction_InitDefaultParams(g_actParam);
}

void Player_Finalize()
{
	//ModelRelease(g_playerModel);
	SkinnedModel_Release(g_playerModel);
	g_playerModel = nullptr;
}

void Player_Update(double elapsedTime)
{
	const bool inputEnabled = !ImGuiManager::IsVisible();
	const float dt = (float)elapsedTime;

	// CrouchJump landing move-lock timer
	if (s_crouchFJumpMoveLockT > 0.0f)
	{
		s_crouchFJumpMoveLockT -= dt;
		if (s_crouchFJumpMoveLockT < 0.0f) s_crouchFJumpMoveLockT = 0.0f;
	}


	static bool  s_prevGrounded = true;
	static bool  s_playLand = false;
	static float s_landT = 0.0f;

	static bool  s_playJump = false;
	static float s_jumpT = 0.0f;

	// ===== Double jump tuning =====
	constexpr float DOUBLE_JUMP_HEIGHT_MULT = 1.5f;          // 目標：高さ
	constexpr float DOUBLE_JUMP_SPEED_MULT = 1.224744871f;  // sqrt(1.5) ＝ 高さ1.5倍相当

	static bool  s_playDoubleJump = false;
	static float s_doubleJumpT = 0.0f;
	bool doubleJumpFiredThisFrame = false;

	static bool s_prevSpin = false;




	XMVECTOR position = XMLoadFloat3(&g_playerPos);
	XMVECTOR velocity = XMLoadFloat3(&g_playerVel);

	// Ground probe（重なってなくても接地を安定させる）
	bool  probedGround = false;
	float groundY = 0.0f;
	const float velY0 = XMVectorGetY(velocity);

	if (velY0 <= 0.01f) // 上昇中は ground 扱いしない
	{
		constexpr float GROUND_EPS = 0.06f;
		if (ProbeGroundY(position, GROUND_EPS, &groundY))
		{
			probedGround = true;
			position = XMVectorSetY(position, groundY);
			if (velY0 < 0.0f) velocity = XMVectorSetY(velocity, 0.0f);
		}
	}

	const bool prevGround = probedGround;




	// ===== Input =====
	PlayerInput in{};

	if (g_inputOverride)
	{
		in = g_overrideInput; // Gamepad / MotionLab
	}
	else if (inputEnabled)
	{
		// Keyboard fallback (not the main path)
		if (KeyLogger_IsPressed(KK_W)) in.moveY += 1.0f;
		if (KeyLogger_IsPressed(KK_S)) in.moveY -= 1.0f;
		if (KeyLogger_IsPressed(KK_D)) in.moveX += 1.0f;
		if (KeyLogger_IsPressed(KK_A)) in.moveX -= 1.0f;

		in.jump = KeyLogger_IsPressed(KK_U);
		in.spin = KeyLogger_IsPressed(KK_I);
		in.crouch = KeyLogger_IsPressed(KK_O);
	}

	// CrouchJump後の硬直：少しの間だけ移動入力を無効化（ジャンプ/スピン等のボタンはそのまま）
	if (s_crouchFJumpMoveLockT > 0.0f && prevGround)
	{
		in.moveX = 0.0f;
		in.moveY = 0.0f;
	}


	const PlayerActionId prevActionId = g_act.id;
	const float rawMoveX = in.moveX;
	const float rawMoveY = in.moveY;

	// ===== Landing-timed 2nd jump (input buffer & window timer) =====
	const bool jumpTrgInput = (in.jump && !s_prevJumpHeldInput);
	s_prevJumpHeldInput = in.jump;

	constexpr float JUMP_BUFFER_TIME = 0.12f;
	if (jumpTrgInput) s_jumpBufferT = JUMP_BUFFER_TIME;
	else             s_jumpBufferT = std::max(0.0f, s_jumpBufferT - dt);

	if (s_doubleJumpWindowT > 0.0f)
		s_doubleJumpWindowT = std::max(0.0f, s_doubleJumpWindowT - dt);


	// ===== Action FSM =====
	PlayerActionInput ai{};
	ai.moveX = in.moveX;
	ai.moveY = in.moveY;
	ai.jumpHeld = in.jump;
	ai.dashHeld = in.dash;
	ai.spinHeld = in.spin;
	ai.crouchHeld = in.crouch;

	PlayerActionSensors sen{};
	sen.onGround = prevGround;
	sen.velY = XMVectorGetY(velocity);



	PlayerActionOutput ao{};
	PlayerAction_Update(g_act, g_actParam, ai, sen, dt, ao);

	// --- Cancel crouch forward jump when spin starts (in air) ---
	{
		const bool isSpin = (g_act.id == PlayerActionId::Spin);
		const bool spinStarted = (isSpin && !s_prevSpin);

		if (spinStarted && g_crouchForwardJumpActive)
		{
			g_crouchForwardJumpActive = false;

			// スピンでCrouchJumpのXZ推進を中止：通常ジャンプ落下と同じ（ベースXZを消す）
			velocity = XMVectorSet(0.0f, XMVectorGetY(velocity), 0.0f, 0.0f);
		}
	}

	const float CROUCH_FORWARD_JUMP_INPUT_MIN = 0.20f;
	const float CROUCH_FORWARD_JUMP_FORWARD_DOT = cosf(DirectX::XMConvertToRadians(60.0f));
	const float CROUCH_FORWARD_JUMP_Y_MULT = 0.9f;
	const float CROUCH_FORWARD_JUMP_SPEED_XZ = 6.5f;
	const float CROUCH_FORWARD_JUMP_ACCEL_TIME = 0.5f;
	const float CROUCH_FORWARD_JUMP_AIR_CONTROL_SCALE = 0.04f;
	const float CROUCH_FORWARD_JUMP_INPUT_SPEED = 1.0f;
	const float CROUCH_FORWARD_JUMP_LAND_MOVE_LOCK = 0.18f;




	const float rawMoveMagSq = (rawMoveX * rawMoveX) + (rawMoveY * rawMoveY);
	const bool hasMoveInput = (rawMoveMagSq > (CROUCH_FORWARD_JUMP_INPUT_MIN * CROUCH_FORWARD_JUMP_INPUT_MIN));
	XMVECTOR crouchJumpDir = XMVectorZero();
	bool hasCrouchJumpDir = false;
	float crouchForwardDot = -1.0f;
	if (hasMoveInput)
	{
		XMVECTOR camFront = XMLoadFloat3(&PlayerCamera_GetFront());
		camFront = XMVectorSetY(camFront, 0.0f);
		if (XMVectorGetX(XMVector3LengthSq(camFront)) < 1.0e-6f)
			camFront = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		camFront = XMVector3Normalize(camFront);

		const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		const XMVECTOR camRight = XMVector3Normalize(XMVector3Cross(up, camFront));

		XMVECTOR dir = camRight * rawMoveX + camFront * rawMoveY;
		const float dirLenSq = XMVectorGetX(XMVector3LengthSq(dir));
		if (dirLenSq > 1.0e-6f)
		{
			dir = XMVector3Normalize(dir);
			hasCrouchJumpDir = true;
			crouchJumpDir = dir;
			XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&g_playerFront));
			crouchForwardDot = XMVectorGetX(XMVector3Dot(front, dir));
		}
	}

	const bool crouchForwardJumpTrg =
		(prevActionId == PlayerActionId::Crouch) && prevGround && jumpTrgInput && hasCrouchJumpDir &&
		(crouchForwardDot >= CROUCH_FORWARD_JUMP_FORWARD_DOT);


	const bool isCrouch = (ao.id == PlayerActionId::Crouch);
	if (isCrouch)
	{
		in.moveX = 0.0f;
		in.moveY = 0.0f;
		if (prevGround)
		{
			velocity = XMVectorSet(0.0f, XMVectorGetY(velocity), 0.0f, 0.0f);
		}
		g_brakeTimer = 0.0f;
		g_dash2AccelDist = 0.0f;
	}

	// Movement scale (crouch / spin etc.)
	const float moveAccel = g_tune.moveAccel * ao.moveSpeedScale;
	const float airControlScale = g_crouchForwardJumpActive ? CROUCH_FORWARD_JUMP_AIR_CONTROL_SCALE : 1.0f;

	// This frame's grounded will be determined by collision resolution.
	g_isGrounded = false;

	// ===== Apply action outputs to velocity / position (type-safe) =====
	if (ao.overrideVelocity)
	{
		// XMFLOAT3 -> XMVECTOR
		velocity = XMLoadFloat3(&ao.velocity);
	}
	else
	{
		// Jump request (set Y speed)
		if (ao.requestJump && prevGround)
		{
			const bool crouchForwardJump = crouchForwardJumpTrg;
			const bool doDouble = (s_doubleJumpWindowT > 0.0f) && !crouchForwardJump;

			float jumpY = doDouble ? (ao.jumpSpeedY * DOUBLE_JUMP_SPEED_MULT) : ao.jumpSpeedY;


			if (crouchForwardJump)
			{
				jumpY = ao.jumpSpeedY * CROUCH_FORWARD_JUMP_Y_MULT;
				const XMVECTOR velXZ = XMVectorSet(XMVectorGetX(velocity), 0.0f, XMVectorGetZ(velocity), 0.0f);
				velocity = XMVectorSet(XMVectorGetX(velXZ), jumpY, XMVectorGetZ(velXZ), 0.0f);
				XMStoreFloat3(&g_playerFront, XMVector3Normalize(crouchJumpDir));
				XMStoreFloat3(&g_crouchForwardJumpDir, XMVector3Normalize(crouchJumpDir));
				g_crouchForwardJumpActive = true;
			}
			else
			{
				velocity = XMVectorSetY(velocity, jumpY);
				g_crouchForwardJumpActive = false;
			}

			// 2段ジャンプを消費
			if (doDouble) s_doubleJumpWindowT = 0.0f;

			if (doDouble) doubleJumpFiredThisFrame = true;


			// 「地上ジャンプ由来で空中にいる」フラグ（着地ウィンドウ開始の元）
			s_airFromGroundJump = true;

			// Variable jump 側も jumpY を基準にする
			if (crouchForwardJump)
			{
				g_varJumpActive = false;
				g_varJumpHoldT = 0.0f;
				g_varJumpStartSpeedY = 0.0f;
				g_varJumpCutApplied = true;
			}
			else
			{
				g_varJumpActive = true;
				g_varJumpHoldT = 0.0f;
				g_varJumpStartSpeedY = jumpY;
				g_varJumpCutApplied = false;
			}


		}

		if (ao.addVelY)
		{
			velocity = XMVectorSetY(velocity, XMVectorGetY(velocity) + ao.velYDelta);
		}





		// Gravity (skip while wall-grabbing, because the action overwrites velY)
		if (!prevGround && !ao.overrideVelY)
		{
			const XMVECTOR gravity = XMVectorSet(0.0f, -g_tune.gravity, 0.0f, 0.0f);
			velocity += gravity * dt;

			// terminal fall (downward is negative)
			if (g_tune.terminalFall < 0.0f && XMVectorGetY(velocity) < g_tune.terminalFall)
			{
				velocity = XMVectorSetY(velocity, g_tune.terminalFall);
			}

			// ---- Spin air-lift : 落下中でも必ず浮く（重力を打ち消す + 落下速度リセット）----
			{
				const bool isSpin = (g_act.id == PlayerActionId::Spin);
				const bool spinStarted = (isSpin && !s_prevSpin);

				if (spinStarted && g_crouchForwardJumpActive)
				{
					g_crouchForwardJumpActive = false;
					velocity = XMVectorSet(0.0f, XMVectorGetY(velocity), 0.0f, 0.0f);
				}

				if (isSpin && !prevGround)
				{
					// ① このフレームに入れた重力を打ち消す（= 落下にかかってる力を0にする）
					velocity = XMVectorSetY(velocity, XMVectorGetY(velocity) + g_tune.gravity * dt);

					// ② 発動した瞬間、落下速度を0にする（どの位置/速度からでも浮く）
					if (spinStarted)
					{
						// 落下中なら止める（上昇中はそのままでもOKだが、統一したければ 0 にしても良い）
						if (XMVectorGetY(velocity) < 0.0f)
							velocity = XMVectorSetY(velocity, 0.0f);
					}

					// ③ 浮きインパルスを「加算」で入れる（上昇中でも落下中でも効く）
					const float liftDur = std::max(1.0e-6f, g_actParam.spinAirLiftDuration); // 例: 0.12f
					if (g_act.timer <= liftDur)
					{
						const float accelY = g_actParam.spinAirLiftImpulseY / liftDur; // 合計で impulseY になる
						velocity = XMVectorSetY(velocity, XMVectorGetY(velocity) + accelY * dt);
					}
				}

				s_prevSpin = isSpin;
			}

		}

		// Vertical speed override (wall slide etc.)
		if (ao.overrideVelY)
		{
			velocity = XMVectorSetY(velocity, ao.velY);
		}
	}

	//Spin
	PlayerActionState* st = Player_GetActionState();
	PlayerActionParams* p = Player_GetActionParams();

	if (st && p && st->id == PlayerActionId::Spin)
	{
		// 0.5秒で1回転（左回り）
		const float angSpeed = XM_2PI / p->spinTime; // rad/s
		g_spinYaw += -angSpeed * dt; // 左回り（逆なら符号を+に）
	}
	else
	{
		g_spinYaw = 0.0f;
	}


	// ===== Move input (XZ) =====
	const bool isAirLike =
		(g_act.id == PlayerActionId::Air) ||
		(!prevGround && g_act.id == PlayerActionId::Spin);

	if (!isCrouch)
	{
		// Camera-based move direction
		XMVECTOR camFront = XMLoadFloat3(&PlayerCamera_GetFront());
		camFront = XMVectorSetY(camFront, 0.0f);
		if (XMVectorGetX(XMVector3LengthSq(camFront)) < 1.0e-6f)
			camFront = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		camFront = XMVector3Normalize(camFront);

		const XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		const XMVECTOR camRight = XMVector3Normalize(XMVector3Cross(up, camFront));

		XMVECTOR dir = camRight * in.moveX + camFront * in.moveY;
		const float dirLenSq = XMVectorGetX(XMVector3LengthSq(dir));
		const bool hasMoveDir = (dirLenSq > 1.0e-6f);

		if (hasMoveDir)
		{
			dir = XMVector3Normalize(dir);

			// ---- Mario move spec (tweak here) ----
			constexpr float WALK_MAX = 1.0f;           // 0-50%
			constexpr float DASH1_MAX = 2.0f;          // 50-75%
			constexpr float DASH2_MAX = 3.0f;          // 75-100%
			constexpr float BRAKE_TIME = 0.30f;        // brake duration
			constexpr float BRAKE_FRICTION = 25.0f;    // how quickly you stop while braking
			constexpr float DASH2_STAGE1_DIST = 1.0f;  // 1 block

			// Analog magnitude (0..1)
			const float magSq = (in.moveX * in.moveX) + (in.moveY * in.moveY);
			const float mag = (magSq >= 1.0f) ? 1.0f : (magSq > 0.0f ? sqrtf(magSq) : 0.0f);

			XMVECTOR velXZ = XMVectorSet(XMVectorGetX(velocity), 0.0f, XMVectorGetZ(velocity), 0.0f);
			float speedXZ = XMVectorGetX(XMVector3Length(velXZ));

			// Start brake: input dir is >=120 deg away from current move dir
			if (g_act.id != PlayerActionId::Air)
			{
				if (g_brakeTimer <= 0.0f && mag > 1.0e-3f && speedXZ > 0.2f)
				{
					XMVECTOR velDir = velXZ * (1.0f / speedXZ);
					float vdot = XMVectorGetX(XMVector3Dot(velDir, dir));
					if (vdot <= -0.5f)
					{
						g_brakeTimer = BRAKE_TIME;
						g_dash2AccelDist = 0.0f;
					}
				}
			}
			else
			{
				// air: never brake
				g_brakeTimer = 0.0f;
			}




			if (!s_playLand)
			{
				if (!isAirLike)
				{
					// ===== 地上（今までのまま）=====
					if (g_brakeTimer > 0.0f)
					{
						g_brakeTimer -= dt;

						static float brakeT = 0.0f;
						brakeT += dt;
						SkinnedModel_UpdateClip(g_playerModel, brakeT, 8, 1.40f, 1.54f, true);

						velXZ += (-velXZ) * (BRAKE_FRICTION * dt);

						if (g_brakeTimer <= 0.0f)
						{
							velXZ = XMVectorZero();
							speedXZ = 0.0f;
						}

						velocity = XMVectorSet(XMVectorGetX(velXZ), XMVectorGetY(velocity), XMVectorGetZ(velXZ), 0.0f);
					}
					else
					{
						static float t = 0.0f;
						t += dt;
						SkinnedModel_Update(g_playerModel, t, 21);

						if (mag >= 0.75f && speedXZ > 0.05f) g_dash2AccelDist += speedXZ * dt;
						else g_dash2AccelDist = 0.0f;

						float desiredSpeed = 0.0f;

						if (mag < 0.5f)
						{
							desiredSpeed = WALK_MAX * (mag / 0.5f);
						}
						else if (mag < 0.75f)
						{
							const float t = (mag - 0.5f) / 0.25f;
							desiredSpeed = WALK_MAX + (DASH1_MAX - WALK_MAX) * t;
						}
						else
						{
							if (g_dash2AccelDist < DASH2_STAGE1_DIST) desiredSpeed = DASH1_MAX;
							else
							{
								const float t = (mag - 0.75f) / 0.25f;
								desiredSpeed = DASH1_MAX + (DASH2_MAX - DASH1_MAX) * t;
							}
						}

						// 地上は向きも回す（今まで通り）
						float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&g_playerFront), dir));
						dot = ClampFloat(dot, -1.0f, 1.0f);

						const float angle = acosf(dot);
						const float rotStep = g_tune.rotSpeed * dt;

						XMVECTOR newFront = dir;
						if (angle >= rotStep)
						{
							const float crossY = XMVectorGetY(XMVector3Cross(XMLoadFloat3(&g_playerFront), dir));
							const float sign = (crossY < 0.0f) ? -1.0f : +1.0f;
							const XMMATRIX r = XMMatrixRotationY(sign * rotStep);
							newFront = XMVector3TransformNormal(XMLoadFloat3(&g_playerFront), r);
						}
						XMStoreFloat3(&g_playerFront, XMVector3Normalize(newFront));

						const XMVECTOR desiredVelXZ = dir * desiredSpeed;
						const XMVECTOR delta = desiredVelXZ - velXZ;
						const float deltaLen = XMVectorGetX(XMVector3Length(delta));
						const float maxDelta = moveAccel * dt;

						if (deltaLen > maxDelta && deltaLen > 1.0e-6f)
							velXZ += delta * (maxDelta / deltaLen);
						else
							velXZ = desiredVelXZ;

						velocity = XMVectorSet(XMVectorGetX(velXZ), XMVectorGetY(velocity), XMVectorGetZ(velXZ), 0.0f);
					}
				}
				else
				{
					// ===== 空中：どの方向へも移動OK（ただし見た目の向きは回さない）=====
					constexpr float AIR_SPEED_MAX = 1.6f;   // 空中最大速度（好みで）
					constexpr float AIR_ACCEL_SCALE = 0.35f;  // 空中加速（地上比）

					// 空中は地上状態を使わない（バグ防止）
					g_brakeTimer = 0.0f;
					g_dash2AccelDist = 0.0f;

					// 入力方向へ「速度ベクトルだけ」寄せる（g_playerFront は更新しない）
					if (g_crouchForwardJumpActive)
					{
						const XMVECTOR jumpDir = XMVector3Normalize(XMLoadFloat3(&g_crouchForwardJumpDir));
						XMVECTOR desiredVelXZ = jumpDir * CROUCH_FORWARD_JUMP_SPEED_XZ;

						if (hasMoveDir)
						{
							desiredVelXZ += dir * (CROUCH_FORWARD_JUMP_INPUT_SPEED * mag);
						}

						const XMVECTOR delta = desiredVelXZ - velXZ;
						const float deltaLen = XMVectorGetX(XMVector3Length(delta));
						const float accel = CROUCH_FORWARD_JUMP_SPEED_XZ / std::max(1.0e-6f, CROUCH_FORWARD_JUMP_ACCEL_TIME);
						const float maxDelta = accel * dt;

						if (deltaLen > maxDelta && deltaLen > 1.0e-6f)
							velXZ += delta * (maxDelta / deltaLen);
						else
							velXZ = desiredVelXZ;

						velocity = XMVectorSet(XMVectorGetX(velXZ), XMVectorGetY(velocity), XMVectorGetZ(velXZ), 0.0f);
					}
					else
					{
						const float desiredSpeed = AIR_SPEED_MAX * mag;
						const XMVECTOR desiredVelXZ = dir * desiredSpeed;

						const XMVECTOR delta = desiredVelXZ - velXZ;
						const float deltaLen = XMVectorGetX(XMVector3Length(delta));
						const float maxDelta = (moveAccel * AIR_ACCEL_SCALE * airControlScale) * dt;

						if (deltaLen > maxDelta && deltaLen > 1.0e-6f)
							velXZ += delta * (maxDelta / deltaLen);
						else
							velXZ = desiredVelXZ;

						velocity = XMVectorSet(XMVectorGetX(velXZ), XMVectorGetY(velocity), XMVectorGetZ(velXZ), 0.0f);
					}
				}
			}



		}

		else if (isAirLike && g_crouchForwardJumpActive)
		{
			// Crouch forward jump keeps XZ momentum even without input.
			XMVECTOR velXZ = XMVectorSet(XMVectorGetX(velocity), 0.0f, XMVectorGetZ(velocity), 0.0f);
			const XMVECTOR jumpDir = XMVector3Normalize(XMLoadFloat3(&g_crouchForwardJumpDir));
			const XMVECTOR desiredVelXZ = jumpDir * CROUCH_FORWARD_JUMP_SPEED_XZ;

			const XMVECTOR delta = desiredVelXZ - velXZ;
			const float deltaLen = XMVectorGetX(XMVector3Length(delta));
			const float accel = CROUCH_FORWARD_JUMP_SPEED_XZ / std::max(1.0e-6f, CROUCH_FORWARD_JUMP_ACCEL_TIME);
			const float maxDelta = accel * dt;

			if (deltaLen > maxDelta && deltaLen > 1.0e-6f)
				velXZ += delta * (maxDelta / deltaLen);
			else
				velXZ = desiredVelXZ;

			velocity = XMVectorSet(XMVectorGetX(velXZ), XMVectorGetY(velocity), XMVectorGetZ(velXZ), 0.0f);
		}

		// 無入力検出（待機アニメ用）
		{
			const float m2 = (in.moveX * in.moveX) + (in.moveY * in.moveY);
			const bool noMoveInput = (m2 <= 1.0e-6f);

			// 速度が残ってる間は「待機」扱いしない（滑ってる最中に idleTotalT を進めない）
			const XMVECTOR velXZ = XMVectorSet(XMVectorGetX(velocity), 0.0f, XMVectorGetZ(velocity), 0.0f);
			const float speedXZ = XMVectorGetX(XMVector3Length(velXZ));

			// 「本当に待機」判定：無入力 + 地上 + ほぼ停止 +（ブレーキ/拘束/空中など除外）
			const bool idleNow =
				noMoveInput &&
				prevGround &&                 // ここは現状のロジックに合わせて prevGround を使う
				(speedXZ <= 0.05f) &&         // ★停止判定（必要なら 0.03f～0.10f で調整）
				(g_brakeTimer <= 0.0f) &&
				(ao.id == PlayerActionId::Ground);

			// 待機に入ってからの累計時間
			static float idleTotalT = 0.0f;
			static bool  wasIdle = false;

			if (idleNow)
			{
				// 無入力(=待機)になった瞬間から累計開始
				if (!wasIdle) idleTotalT = 0.0f;
				wasIdle = true;

				g_dash2AccelDist = 0.0f;
				idleTotalT += dt;

				if (idleTotalT <= 8.0f)
				{
					SkinnedModel_Update(g_playerModel, idleTotalT, 5);
				}
				else if (idleTotalT <= 8.0f + 3.0f)
				{
					const float t = idleTotalT - 8.0f;
					SkinnedModel_Update(g_playerModel, t, 23);
				}
				else
				{
					const float t = idleTotalT - (8.0f + 3.0f);
					SkinnedModel_Update(g_playerModel, t, 14);
				}
			}
			else
			{
				wasIdle = false;
				idleTotalT = 0.0f;
			}
		}

	}



	// ===== Friction (XZ only) =====
	{
		const float inMagSq = (in.moveX * in.moveX) + (in.moveY * in.moveY);
		const bool hasInput = (inMagSq > 1.0e-6f);

		float fric = g_tune.friction;
		if (isAirLike) fric = 0.0f;
		if (hasInput) fric *= 0.15f;      // 入力中は滑りやすく（加速が勝つ）
		if (g_brakeTimer > 0.0f) fric = 0.0f; // ブレーキ中は上で減衰させてる

		XMVECTOR velXZ = XMVectorSet(XMVectorGetX(velocity), 0.0f, XMVectorGetZ(velocity), 0.0f);
		velXZ += (-velXZ) * (fric * dt);
		velocity = XMVectorSet(XMVectorGetX(velXZ), XMVectorGetY(velocity), XMVectorGetZ(velXZ), 0.0f);
	}

	// ===== Variable jump: 3-step by A hold time =====
// Strong : hold >= 0.20s  (no cut)
// Medium : 0.10s..0.20s  (cut to mid speed on release)
// Weak   : < 0.10s       (cut to weak speed on release)
	if (g_varJumpActive && !ao.overrideVelocity && !ao.overrideVelY)
	{
		constexpr float HOLD_WEAK = 0.20f;
		constexpr float HOLD_STRONG = 0.40f;
		constexpr float MID_RATIO = 0.80f;
		constexpr float WEAK_RATIO = 0.55f;
		constexpr float EPS = 1.0e-4f;

		// Accumulate hold time while the button is held (clamp at 0.20s).
		if (!g_varJumpCutApplied)
		{
			if (in.jump)
			{
				g_varJumpHoldT += dt;
				if (g_varJumpHoldT > HOLD_STRONG) g_varJumpHoldT = HOLD_STRONG;
			}
			else
			{
				// Released: apply one-time cut if the hold was short.
				if (g_varJumpHoldT < HOLD_STRONG - EPS)
				{
					float cutSpeed = g_varJumpStartSpeedY;

					// weak / medium decision by hold time
					if (g_varJumpHoldT < HOLD_WEAK) cutSpeed = g_varJumpStartSpeedY * WEAK_RATIO;
					else                            cutSpeed = g_varJumpStartSpeedY * MID_RATIO;

					const float vy = XMVectorGetY(velocity);
					if (vy > 0.0f && vy > cutSpeed)
						velocity = XMVectorSetY(velocity, cutSpeed);
				}
				g_varJumpCutApplied = true;
			}
		}

		// Stop tracking after the peak (falling or flat).
		if (XMVectorGetY(velocity) <= 0.0f)
			g_varJumpActive = false;
	}


	// ===== Integrate =====
	position += velocity * dt;

	// ===== Spin AABB vs AABB : Spin attack destroys kind==0 cube  (runtime only)=====
	{
		const PlayerActionState* act = Player_GetActionState();
		if (act && act->id == PlayerActionId::Spin)
		{
			constexpr float SPIN_EXPAND_XZ = 0.4f; // keep in sync with Player_GetSpinAABB default
			AABB spinAabb = Player_ConvertPositionToAABB(position);
			spinAabb.min.x -= SPIN_EXPAND_XZ;
			spinAabb.max.x += SPIN_EXPAND_XZ;
			spinAabb.min.y += 0.2f;
			spinAabb.min.z -= SPIN_EXPAND_XZ;
			spinAabb.max.z += SPIN_EXPAND_XZ;

			const int blockCount = Stage01_GetCount();
			for (int i = 0; i < blockCount; ++i)
			{
				StageBlock* obj = Stage01_GetMutable(i);
				if (!obj) continue;
				if (obj->kind != 10) continue;//kind==１０のCubeにスピンを当てたら破壊できる
				if (!Collision_IsOverlapAABB(spinAabb, obj->aabb)) continue;

				const DirectX::XMFLOAT3 hitPosition{
				(obj->aabb.min.x + obj->aabb.max.x) * 0.5f,
				(obj->aabb.min.y + obj->aabb.max.y) * 0.5f,
				(obj->aabb.min.z + obj->aabb.max.z) * 0.5f
				};
				StageSimpleManager_AddSpinBreakBillboard(hitPosition);

				obj->sizeOffset.x = -obj->size.x;
				obj->sizeOffset.y = -obj->size.y;
				obj->sizeOffset.z = -obj->size.z;
				HideStageBlockRuntime(i);
				break;
			}
		}
	}


	// ===== AABB vs AABB : Player を Cube から押し戻す =====
	{
		for (int solve = 0; solve < 4; ++solve)
		{
			bool anyHit = false;
			bool removedBlock = false;

			for (int i = 0; i < Stage01_GetCount(); ++i)
			{
				const StageBlock* obj = Stage01_Get(i);
				if (!obj) continue;

				AABB playerAabb = Player_ConvertPositionToAABB(position);

				const AABB& box = obj->aabb;

				if (!Collision_IsOverlapAABB(box, playerAabb)) continue;

				const float ox = std::min(playerAabb.max.x, box.max.x) - std::max(playerAabb.min.x, box.min.x);
				const float oy = std::min(playerAabb.max.y, box.max.y) - std::max(playerAabb.min.y, box.min.y);
				const float oz = std::min(playerAabb.max.z, box.max.z) - std::max(playerAabb.min.z, box.min.z);

				if (ox <= 0 || oy <= 0 || oz <= 0) continue;

				const float pcx = (playerAabb.min.x + playerAabb.max.x) * 0.5f;
				const float pcy = (playerAabb.min.y + playerAabb.max.y) * 0.5f;
				const float pcz = (playerAabb.min.z + playerAabb.max.z) * 0.5f;

				const float bcx = (box.min.x + box.max.x) * 0.5f;
				const float bcy = (box.min.y + box.max.y) * 0.5f;
				const float bcz = (box.min.z + box.max.z) * 0.5f;

				if (ox <= oy && ox <= oz)
				{
					const float dir = (pcx < bcx) ? -1.0f : +1.0f;
					position = XMVectorSetX(position, XMVectorGetX(position) + dir * ox);
					velocity = XMVectorSetX(velocity, 0.0f);
				}
				else if (oy <= ox && oy <= oz)
				{
					const float dir = (pcy < bcy) ? -1.0f : +1.0f;
					position = XMVectorSetY(position, XMVectorGetY(position) + dir * oy);

					// Landed on top (only if falling or stopped)
					if (dir > 0.0f && XMVectorGetY(velocity) <= 0.0f)
					{
						g_isGrounded = true;
					}

					// Hit head (jumping) : remove kind==0 cube(runtime only)
					if (dir < 0.0f && XMVectorGetY(velocity) > 0.0f && obj->kind == 10)
					{
						HideStageBlockRuntime(i);
						removedBlock = true;
						anyHit = true;
						velocity = XMVectorSetY(velocity, 0.0f);
						break;
					}

					velocity = XMVectorSetY(velocity, 0.0f);
				}
				else
				{
					const float dir = (pcz < bcz) ? -1.0f : +1.0f;
					position = XMVectorSetZ(position, XMVectorGetZ(position) + dir * oz);
					velocity = XMVectorSetZ(velocity, 0.0f);
				}

				anyHit = true;
			}

			if (removedBlock)
			{
				continue;
			}

			if (!anyHit) break;
		}
	}

	// overlapが無い「ぴったり接地」でも grounded を維持
	if (!g_isGrounded && XMVectorGetY(velocity) <= 0.01f)
	{
		float groundY = 0.0f;
		constexpr float GROUND_EPS = 0.06f;
		if (ProbeGroundY(position, GROUND_EPS, &groundY))
		{
			g_isGrounded = true;
			position = XMVectorSetY(position, groundY);
			velocity = XMVectorSetY(velocity, 0.0f);
		}
	}

	// ===== Landing-timed 2nd jump (landing window & immediate trigger) =====
	const bool landedNow = (!s_prevGrounded && g_isGrounded);
	if (landedNow)
	{
		if (s_airFromGroundJump)
		{
			constexpr float DOUBLE_JUMP_WINDOW = 0.14f;
			s_doubleJumpWindowT = DOUBLE_JUMP_WINDOW;
			s_airFromGroundJump = false;
		}
		// Landed from CrouchJump: stop XZ and lock move input for a short time
		if (g_crouchForwardJumpActive)
		{
			s_crouchFJumpMoveLockT = std::max(s_crouchFJumpMoveLockT, CROUCH_FORWARD_JUMP_LAND_MOVE_LOCK);
			velocity = XMVectorSet(0.0f, XMVectorGetY(velocity), 0.0f, 0.0f);
			g_brakeTimer = 0.0f;
			g_dash2AccelDist = 0.0f;
		}

	}

	if (g_isGrounded && (s_doubleJumpWindowT > 0.0f) && (s_jumpBufferT > 0.0f))
	{
		const float jumpY = g_actParam.jumpSpeedY * DOUBLE_JUMP_SPEED_MULT;


		velocity = XMVectorSetY(velocity, jumpY);
		g_isGrounded = false;

		s_doubleJumpWindowT = 0.0f;
		s_jumpBufferT = 0.0f;

		s_airFromGroundJump = true;

		g_varJumpActive = true;
		g_varJumpHoldT = 0.0f;
		g_varJumpStartSpeedY = jumpY;
		g_varJumpCutApplied = false;

		doubleJumpFiredThisFrame = true;
	}


	// Landed: reset variable-jump tracking
	if (g_isGrounded)
	{
		g_varJumpActive = false;
		g_varJumpHoldT = 0.0f;
		g_varJumpStartSpeedY = 0.0f;
		g_varJumpCutApplied = false;
		g_crouchForwardJumpActive = false;
	}

	// ===== Animation (最終決定はここでやる) =====
	{
		// ===== Spin : Tポーズ固定 + 見た目回転 =====
		static float s_spinVisT = 0.0f;

		static float s_crouchT = 0.0f;
		static bool s_prevCrouch = false;

		static float s_crouchForwardJumpT = 0.0f;
		static bool s_playCrouchForwardJump = false;

		if (g_act.id == PlayerActionId::Spin)
		{
			s_prevCrouch = false;
			s_crouchT = 0.0f;
			// 0.5秒で1回転（左回り）
			constexpr float SPIN_TIME = 0.5f;
			const float angSpeed = DirectX::XM_2PI / SPIN_TIME; // rad/s

			s_spinVisT += dt;
			g_spinYaw += -angSpeed * dt; // 左回り（逆なら + に）

			// Tポーズ（読み込み時ポーズ）に戻す：このフレームの最終出力にする
			SkinnedModel_ResetPose(g_playerModel);

			// スピン中はジャンプ/着地の見た目補正は切る（ズレ防止）
			g_visFixLand = false;
			g_visFixJump = false;
			g_visFixCrouchForwardJump2 = false;

			// ここで終わり：下のジャンプ/着地アニメに行かせない
			s_prevGrounded = g_isGrounded;
		}
		else if (g_act.id == PlayerActionId::Crouch)
		{
			if (!s_prevCrouch) s_crouchT = 0.0f;
			s_crouchT += dt;

			constexpr int ANIM_CROUCH = 7;
			constexpr float CROUCH_CLIP_START = 0.19f;
			constexpr float CROUCH_CLIP_END = 0.39f;
			SkinnedModel_UpdateClip(g_playerModel, s_crouchT, ANIM_CROUCH,
				CROUCH_CLIP_START, CROUCH_CLIP_END, true);

			const bool crouchFJumpStartThisFrame = (ao.requestJump && prevGround && crouchForwardJumpTrg);

			if (crouchFJumpStartThisFrame)
			{
				g_visFixLand = false;
				g_visFixJump = false;
				g_visFixCrouchForwardJump2 = false;
			}
			else
			{
				g_visFixLand = false;
				g_visFixJump = false;
				g_visFixCrouchForwardJump2 = false;
			}

			s_prevGrounded = g_isGrounded;
			s_prevCrouch = true;
		}
		else
		{
			s_prevCrouch = false;
			s_crouchT = 0.0f;
			// スピンじゃない時は回転リセット
			s_spinVisT = 0.0f;
			g_spinYaw = 0.0f;
			if (crouchForwardJumpTrg)
			{
				s_playCrouchForwardJump = true;
				s_crouchForwardJumpT = 0.0f;
			}
			if (g_isGrounded)
			{
				s_playCrouchForwardJump = false;
				s_crouchForwardJumpT = 0.0f;
			}
			// AirからGround になった瞬間を「着地」とみなす
			const bool landed = (!s_prevGrounded && g_isGrounded);
			if (landed)
			{
				s_playLand = true;
				s_landT = 0.0f;
			}

			// ジャンプ開始（このフレームでジャンプ要求が通った瞬間）
			if (ao.requestJump && prevGround && !crouchForwardJumpTrg)
			{
				s_playJump = true;
				s_jumpT = 0.0f;
			}

			// 2段ジャンプ開始（このフレームで2段ジャンプが発動した瞬間）
			if (doubleJumpFiredThisFrame)
			{
				s_playDoubleJump = true;
				s_doubleJumpT = 0.0f;

				// 2段ジャンプを通常ジャンプより優先したいので止める
				s_playJump = false;
			}


			// 空中にいる間はジャンプアニメ扱い（最後停止）
			if (!g_isGrounded) s_playJump = true;
			// 優先度：着地 > ジャンプ > 地上通常
			constexpr int ANIM_JUMP = 22;
			constexpr float JUMP_CLIP_START = 0.28f;
			constexpr float JUMP_CLIP_END = 0.44f;

			constexpr int   ANIM_LAND = 18;
			constexpr float LAND_CLIP_START = 0.00f;
			constexpr float LAND_CLIP_END = 0.45f;
			constexpr float LAND_SHOW_TIME = 0.45f;

			constexpr float CROUCH_FJUMP_CLIP2_START = 0.00f;
			constexpr float CROUCH_FJUMP_CLIP2_END = 0.01f;

			//このフレームで適用した方を覚える（描画オフセット用）
			bool playCrouchForwardJumpThisFrame = false;
			bool playLandThisFrame = false;
			bool playJumpThisFrame = false;



			if (s_playCrouchForwardJump && g_crouchForwardJumpActive && !g_isGrounded)
			{
				playCrouchForwardJumpThisFrame = true;

				const float clip2Len = (CROUCH_FJUMP_CLIP2_END - CROUCH_FJUMP_CLIP2_START);

				s_crouchForwardJumpT += dt;
				SkinnedModel_UpdateClip(g_playerModel, s_crouchForwardJumpT, 4,
					CROUCH_FJUMP_CLIP2_START, CROUCH_FJUMP_CLIP2_END, true);
				if (clip2Len <= 0.0f)
					s_crouchForwardJumpT = 0.0f;

				// 着地中にジャンプが混ざらないように保険
				s_playLand = false;
				s_playJump = false;
				s_playDoubleJump = false;
			}
			else
			{
				// 着地してて着地アニメでもないならジャンプは消す
				if (s_playLand)
				{
					playLandThisFrame = true;

					s_landT += dt;
					//SkinnedModel_UpdateAtTime(g_playerModel, s_landT, ANIM_LAND);
					SkinnedModel_UpdateClip(g_playerModel, s_landT, ANIM_LAND, LAND_CLIP_START, LAND_CLIP_END, true);

					if (s_landT >= LAND_SHOW_TIME) s_playLand = false;

					s_playJump = false;
				}
				else
				{
					if (g_isGrounded) s_playJump = false;

					if (s_playJump)
					{
						playJumpThisFrame = true;

						s_jumpT += dt;
						SkinnedModel_UpdateClip(g_playerModel, s_jumpT, ANIM_JUMP,
							JUMP_CLIP_START, JUMP_CLIP_END, true);
					}
				}
			}


			//オフセット判定は「このフレームに何を描いたか」で決める
			g_visFixLand = playLandThisFrame;
			g_visFixJump = playJumpThisFrame || playCrouchForwardJumpThisFrame;
			g_visFixCrouchForwardJump2 = playCrouchForwardJumpThisFrame;
		}

		s_prevGrounded = g_isGrounded;
	}


	XMStoreFloat3(&g_playerPos, position);
	XMStoreFloat3(&g_playerVel, velocity);
}

void Player_Draw()
{
	Light_SetSpecularWorld(Camera_GetPosition(), 4.0f, { 0.2f,0.2f,0.2f,1.0f });

	float angleX = 90.0f;
	float angleY = 0.0f;
	
	//-atan2f(g_playerFront.z, g_playerFront.x) + XMConvertToRadians(270);
	//XMMATRIX r = XMMatrixRotationX(XMConvertToRadians(angleX)) * XMMatrixRotationY(XMConvertToRadians(angleY));
	float angle = -atan2f(g_playerFront.z, g_playerFront.x) + XMConvertToRadians(270);
	angle += g_spinYaw;
	XMMATRIX r = XMMatrixRotationX(XMConvertToRadians(angleX))*XMMatrixRotationY(angle);

	// ===== Visual-only fixed offset =====
	float fix = 0.0f;
	if (g_visFixLand)      fix = g_visLandForwardFix;
	else if (g_visFixCrouchForwardJump2) fix = g_visCrouchForwardJump2Fix;
	else if (g_visFixJump) fix = g_visJumpForwardFix;

	XMVECTOR pos = XMLoadFloat3(&g_playerPos);
	if (fix != 0.0f)
	{
		XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&g_playerFront));
		pos += front * fix; // 前方向へ固定距離だけずらす
	}

	XMMATRIX t = XMMatrixTranslation(
		XMVectorGetX(pos),
		XMVectorGetY(pos) + PLAYER_DRAW_Y_OFFSET,
		XMVectorGetZ(pos)
	);

	XMMATRIX s = XMMatrixScaling(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
	XMMATRIX world = s * r * t;

	SkinnedModel_Draw(g_playerModel, world);
}

void Player_DepthDraw()
{
	Light_SetSpecularWorld(Camera_GetPosition(), 4.0f, { 0.2f,0.2f,0.2f,1.0f });


	float angle = -atan2f(g_playerFront.z, g_playerFront.x) + XMConvertToRadians(270);
	angle += g_spinYaw;
	XMMATRIX r = XMMatrixRotationY(angle);
	// ===== Visual-only fixed offset =====
	float fix = 0.0f;
	if (g_visFixLand)      fix = g_visLandForwardFix;
	else if (g_visFixCrouchForwardJump2) fix = g_visCrouchForwardJump2Fix;
	else if (g_visFixJump) fix = g_visJumpForwardFix;

	XMVECTOR pos = XMLoadFloat3(&g_playerPos);
	if (fix != 0.0f)
	{
		XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&g_playerFront));
		pos += front * fix;
	}

	XMMATRIX t = XMMatrixTranslation(
		XMVectorGetX(pos),
		XMVectorGetY(pos) + PLAYER_DRAW_Y_OFFSET,
		XMVectorGetZ(pos)
	);

	XMMATRIX s = XMMatrixScaling(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
	XMMATRIX world = s * r * t;

	SkinnedModel_DepthDraw(g_playerModel, world);;
}

const DirectX::XMFLOAT3& Player_GetPosition()
{
	return g_playerPos;
}

const DirectX::XMFLOAT3& Player_GetFront()
{
	return g_playerFront;
}

AABB Player_GetAABB()
{
	return {
		{ g_playerPos.x - PLAYER_HALF_WIDTH,
		  g_playerPos.y,
		  g_playerPos.z - PLAYER_HALF_DEPTH },

		{ g_playerPos.x + PLAYER_HALF_WIDTH,
		  g_playerPos.y + PLAYER_HEIGHT,
		  g_playerPos.z + PLAYER_HALF_DEPTH }
	};
}

AABB Player_ConvertPositionToAABB(const DirectX::XMVECTOR& position)
{
	XMFLOAT3 p; XMStoreFloat3(&p, position);
	AABB aabb{};
	aabb.min = { p.x - PLAYER_HALF_WIDTH,
				 p.y,
				 p.z - PLAYER_HALF_DEPTH };
	aabb.max = { p.x + PLAYER_HALF_WIDTH,
				 p.y + PLAYER_HEIGHT,
				 p.z + PLAYER_HALF_DEPTH };
	return aabb;
}

const DirectX::XMFLOAT3& Player_GetVelocity()
{
	return g_playerVel;
}

PlayerActionParams* Player_GetActionParams()
{
	return &g_actParam;
}

PlayerActionState* Player_GetActionState()
{
	return &g_act;
}

XMFLOAT3 Player_GetDestroyedBrickBlockPosition(StageBlock* o)
{
	XMFLOAT3 pos = o->position;
	return pos;
}

