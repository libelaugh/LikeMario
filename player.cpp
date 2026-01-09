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
#include "stage01_manage.h"
// #include "stage_map.h"  
#include"collision.h"
#include "debug_ostream.h"
#include "imgui_manager.h"
#include "gamepad.h"
#include "player_sensors.h"
#include "player_action.h"
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

// ===== Wall / Ledge sensors =====
static bool g_isWallTouch = false;
static DirectX::XMFLOAT3 g_wallNormal{ 0,0,0 }; // wall -> player (axis)

static bool g_isLedgeAvailable = false;
static DirectX::XMFLOAT3 g_ledgeHangPos{ 0,0,0 }; // snap position (center)
static DirectX::XMFLOAT3 g_ledgeNormal{ 0,0,0 };  // wall -> player (axis)

static PlayerLedgeTuning g_ledgeTune{}; // ImGuiで調整する用

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

// 調整値（ワールド単位）: 前にズレるなら「マイナス」で後ろに引く
static float g_visJumpForwardFix = -3.0f; // 例: -0.10 ～ -0.30 あたりで調整
static float g_visLandForwardFix = -1.0f; // 例



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

	static bool  s_prevGrounded = true;
	static bool  s_playLand = false;
	static float s_landT = 0.0f;

	static bool  s_playJump = false;
	static float s_jumpT = 0.0f;


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

		in.jump = KeyLogger_IsPressed(KK_J);
		// in.dash / in.spin / in.crouch : map if you want
	}

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
	sen.velY = g_playerVel.y;

	sen.wallTouch = g_isWallTouch;
	sen.wallNormal = g_wallNormal;

	sen.ledgeAvailable = g_isLedgeAvailable;
	sen.ledgeHangPos = g_ledgeHangPos;
	sen.ledgeNormal = g_ledgeNormal;

	PlayerActionOutput ao{};
	PlayerAction_Update(g_act, g_actParam, ai, sen, dt, ao);

	// Movement scale (crouch / spin etc.)
	const float moveAccel = g_tune.moveAccel * ao.moveSpeedScale;

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
			velocity = XMVectorSetY(velocity, ao.jumpSpeedY);

			// Variable jump: start tracking hold time (A button)
			g_varJumpActive = true;
			g_varJumpHoldT = 0.0f;
			g_varJumpStartSpeedY = ao.jumpSpeedY; // strong
			g_varJumpCutApplied = false;
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
		}

		// Vertical speed override (wall slide etc.)
		if (ao.overrideVelY)
		{
			velocity = XMVectorSetY(velocity, ao.velY);
		}
	}

	// Lock XZ movement if requested (wall grab / ledge hang)
	if (ao.lockMoveXZ)
	{
		velocity = XMVectorSetX(velocity, 0.0f);
		velocity = XMVectorSetZ(velocity, 0.0f);
	}

	// ===== Move input (XZ) =====
	if (!ao.lockMoveXZ)
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

		if (dirLenSq > 1.0e-6f)
		{
			if (dirLenSq > 1.0e-6f)
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
				// cos(120°) = -0.5
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

				if (!s_playLand && g_act.id != PlayerActionId::Air)
				{
					if (g_brakeTimer > 0.0f)
					{
						g_brakeTimer -= dt;

						//SkinnedModel_UpdateAtTime(g_playerModel, 1.54f, 8);
						static float brakeT = 0.0f;
						brakeT += dt;
						SkinnedModel_UpdateClip(g_playerModel, brakeT, 8, 1.40f, 1.54f, true);

						// strong damping during brake
						velXZ += (-velXZ) * (BRAKE_FRICTION * dt);

						if (g_brakeTimer <= 0.0f)
						{
							velXZ = XMVectorZero(); // stop after 0.3s
							speedXZ = 0.0f;
						}

						velocity = XMVectorSet(XMVectorGetX(velXZ), XMVectorGetY(velocity), XMVectorGetZ(velXZ), 0.0f);
					}
					else
					{
						static float t = 0.0f;
						t += dt;
						SkinnedModel_Update(g_playerModel, t, 21);

						// 2-stage accel for dash2:
						// first 1 block -> DASH1, then allow DASH2
						if (mag >= 0.75f && speedXZ > 0.05f) g_dash2AccelDist += speedXZ * dt;
						else g_dash2AccelDist = 0.0f;

						// Decide desired speed by stick magnitude
						float desiredSpeed = 0.0f;

						if (mag < 0.5f)
						{
							// 0-50% : slow walk
							desiredSpeed = WALK_MAX * (mag / 0.5f);
						}
						else if (mag < 0.75f)
						{
							// 50-75% : dash1 (smooth blend from walk->dash1)
							const float t = (mag - 0.5f) / 0.25f;
							desiredSpeed = WALK_MAX + (DASH1_MAX - WALK_MAX) * t;
						}
						else
						{
							// 75-100% : dash2
							if (g_dash2AccelDist < DASH2_STAGE1_DIST)
							{
								desiredSpeed = DASH1_MAX; // stage1 for 1 block
							}
							else
							{
								const float t = (mag - 0.75f) / 0.25f;
								desiredSpeed = DASH1_MAX + (DASH2_MAX - DASH1_MAX) * t;
							}
						}

						// Rotate player front toward desired dir (smooth curve while <=120°)
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

						// MoveTowards velocity to desired velocity (accel)
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
			}

		}
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
			!ao.lockMoveXZ &&
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



	// ===== Friction (XZ only) =====
	{
		const float inMagSq = (in.moveX * in.moveX) + (in.moveY * in.moveY);
		const bool hasInput = (inMagSq > 1.0e-6f) && !ao.lockMoveXZ;

		float fric = g_tune.friction;
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

	// Position snap (ledge hang)
	if (ao.overridePosition)
	{
		position = XMLoadFloat3(&ao.position);
	}

	// ===== AABB vs AABB : Player を Cube から押し戻す =====
	{
		for (int solve = 0; solve < 4; ++solve)
		{
			bool anyHit = false;

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


	// Landed: reset variable-jump tracking
	if (g_isGrounded)
	{
		g_varJumpActive = false;
		g_varJumpHoldT = 0.0f;
		g_varJumpStartSpeedY = 0.0f;
		g_varJumpCutApplied = false;
	}


	// ===== Build wall/ledge sensors (post-resolve) =====
	{
		AABB playerAabb = Player_ConvertPositionToAABB(position);
		const float velY = XMVectorGetY(velocity);
		const bool onGround = g_isGrounded;

		PlayerWallLedgeSensors s{};
		PlayerSensors_BuildWallLedge(playerAabb, velY, onGround, g_ledgeTune, s);

		g_isWallTouch = s.wallTouch;
		g_wallNormal = s.wallNormal;

		g_isLedgeAvailable = s.ledgeAvailable;
		g_ledgeHangPos = s.ledgeHangPos;
		g_ledgeNormal = s.ledgeNormal;
	}

	// ===== Animation (最終決定はここでやる) =====
	{
		// AirからGround になった瞬間を「着地」とみなす
		const bool landed = (!s_prevGrounded && g_isGrounded);
		if (landed)
		{
			s_playLand = true;
			s_landT = 0.0f;
		}

		// ジャンプ開始（このフレームでジャンプ要求が通った瞬間）
		if (ao.requestJump && prevGround)
		{
			s_playJump = true;
			s_jumpT = 0.0f;
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

		//このフレームで適用した方を覚える（描画オフセット用）
		bool playLandThisFrame = false;
		bool playJumpThisFrame = false;

		if (s_playLand)
		{
			playLandThisFrame = true;

			s_landT += dt;
			//SkinnedModel_UpdateAtTime(g_playerModel, s_landT, ANIM_LAND);
			SkinnedModel_UpdateClip(g_playerModel, s_landT, ANIM_LAND, LAND_CLIP_START, LAND_CLIP_END, true);

			// 終了判定はしてOK（ただしオフセットは playLandThisFrame で維持）
			if (s_landT >= LAND_SHOW_TIME) s_playLand = false;

			// 着地中にジャンプが混ざらないように保険
			s_playJump = false;
		}
		else
		{
			// 着地してて着地アニメでもないならジャンプは消す
			if (g_isGrounded) s_playJump = false;

			if (s_playJump)
			{
				playJumpThisFrame = true;

				s_jumpT += dt;
				SkinnedModel_UpdateClip(g_playerModel, s_jumpT, ANIM_JUMP,
					JUMP_CLIP_START, JUMP_CLIP_END, true);
			}
		}

		//オフセット判定は「このフレームに何を描いたか」で決める
		g_visFixLand = playLandThisFrame;
		g_visFixJump = playJumpThisFrame;

		s_prevGrounded = g_isGrounded;
	}


	XMStoreFloat3(&g_playerPos, position);
	XMStoreFloat3(&g_playerVel, velocity);
}


/*
void Player_Update(double elapsedTime)
{
	const bool inputEnabled = !ImGuiManager::IsVisible();

	XMVECTOR position = XMLoadFloat3(&g_playerPos);//演算できないXMLoadFloatから演算できるXMVECTORにする
	XMVECTOR velocity = XMLoadFloat3(&g_playerVel);
	XMVECTOR prevPos = position;                   // ← 衝突時に戻す用
	XMVECTOR gvelocity = {};

	PlayerInput in{};

	if (g_inputOverride)
	{
		in = g_overrideInput;//以降、in を使って移動/ジャンプ/スピン処理する

		// ===== Action FSM =====
		PlayerActionInput ai{};
		ai.moveX = in.moveX;
		ai.moveY = in.moveY;
		ai.jumpHeld = in.jump;
		ai.dashHeld = in.dash;
		ai.spinHeld = in.spin;
		ai.crouchHeld = in.crouch;

		PlayerActionSensors sen{};
		sen.onGround = g_isGrounded;
		sen.velY = g_playerVel.y;

		sen.wallTouch = g_isWallTouch;
		sen.wallNormal = g_wallNormal;

		sen.ledgeAvailable = g_isLedgeAvailable;
		sen.ledgeHangPos = g_ledgeHangPos;
		sen.ledgeNormal = g_ledgeNormal;

		PlayerActionOutput ao{};
		PlayerAction_Update(g_act, g_actParam, ai, sen, elapsedTime, ao);

		// しゃがみ等の速度倍率
		const float moveAccel = g_tune.moveAccel * ao.moveSpeedScale;

		if (ao.lockMoveXZ)
		{
			// 例：このフレームはXZ加速/方向更新をスキップ
			// skipMoveXZ = true;
		}

		// ジャンプ要求
		if (ao.requestJump)
		{
			velocity = XMVectorSetY(velocity, ao.jumpSpeedY);
			g_isGrounded = false;
		}

		// 壁掴みスライド（Y速度上書き）
		if (ao.overrideVelY)
		{
			velocity = XMVectorSetY(velocity, ao.velY);
		}

		// 速度丸ごと上書き（壁ジャンプ/崖からジャンプ）
		if (ao.overrideVelocity)
		{
			velocity = XMLoadFloat3(&ao.velocity);
		}

		// 位置スナップ（崖掴み固定）
		if (ao.overridePosition)
		{
			position = XMLoadFloat3(&ao.position);
		}


	}
	else
	{
		//ジャンプ
		if (inputEnabled && KeyLogger_IsTrigger(KK_J) && !g_isGrounded) {
			velocity += {0.0f, g_tune.jumpImpulse, 0.0f};//ジャンプ力
			g_isGrounded = true;
		}

		//重力に引かれれ落ちる
		XMVECTOR gravity = DirectX::XMVectorSet(0.0f, -g_tune.gravity, 0.0f, 0.0f);
		velocity += gravity * (float)elapsedTime;
		gvelocity = velocity * (float)elapsedTime;


		XMVECTOR direction{};
		XMVECTOR front = XMLoadFloat3(&PlayerCamera_GetFront()) * XMVECTOR { 1.0f, 0.0f, 1.0f };
		if (inputEnabled)
		{
			if (KeyLogger_IsPressed(KK_W)) {
				direction += front;
				direction = XMVector3Normalize(direction);
			}
			if (KeyLogger_IsPressed(KK_S)) {
				direction -= front;
				direction = XMVector3Normalize(direction);
			}
			if (KeyLogger_IsPressed(KK_A)) {
				direction -= XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
				direction = XMVector3Normalize(direction);
			}
			if (KeyLogger_IsPressed(KK_D)) {
				direction += XMVector3Cross({ 0.0f,1.0f,0.0f }, front);
				direction = XMVector3Normalize(direction);
			}
		}


		if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f) {
			direction = XMVector3Normalize(direction);

			//2つのベクトルのなす角は
			float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&g_playerFront), direction));
			// ★acosの安全ガード（わずかな誤差でNaN防止）
			if (dot > 1.0f) dot = 1.0f;
			if (dot < -1.0f) dot = -1.0f;

			float angle = acosf(dot);

			//移動時のプレイヤーの回転モーション(前ベクトルを変えて回転）
			const float ROTATION_SPEED = g_tune.rotSpeed * (float)elapsedTime;//回転速度

			if (angle < ROTATION_SPEED) {
				front = direction;
			}
			else {
				//向きたい方向が右回りか左回りか 
				XMMATRIX r = XMMatrixIdentity();

				if (XMVectorGetY(XMVector3Cross(XMLoadFloat3(&g_playerFront), direction)) < 0.0f) {
					r = XMMatrixRotationY(-ROTATION_SPEED);//プレイヤー方向の外積のY成分の正負によって回転方向を決める
				}
				else {
					r = XMMatrixRotationY(ROTATION_SPEED);
				}

				front = XMVector3TransformNormal(XMLoadFloat3(&g_playerFront), r);
			}
			velocity += front * (float)(g_tune.moveAccel * elapsedTime);
			XMStoreFloat3(&g_playerFront, front);
		}

		velocity += -velocity * (float)(g_tune.friction * elapsedTime);//抵抗

		//位置の積分はここで1回だけ
		position += velocity * (float)elapsedTime;//最終Position
	}

	// 面（y=0）をクランプ：打ち消しではなく固定＆落下速度ゼロ化
	/*if (XMVectorGetY(position) < 0.0f) {
		position = XMVectorSetY(position, 0.0f);
		velocity = XMVectorSetY(velocity, 0.0f);
		g_isJump = false;
	}

	if (XMVectorGetY(position) < 0.0f) {
		position -= gvelocity;
		//gvelocity={};
		velocity *= { 1.0f, 0.0f, 1.0f };
		g_isJump = false;
	}*/

	// ===== AABB vs AABB : Player を Cube から押し戻す =====
/*
	{
		// 何個か重なってても抜けられるように、最大数回 解決（簡易）
		for (int solve = 0; solve < 4; ++solve)
		{
			bool anyHit = false;

			AABB playerAabb = Player_ConvertPositionToAABB(position);

			for (int i = 0; i < Stage01_GetCount(); ++i)
			{
				const StageBlock* obj = Stage01_Get(i);
				if (!obj) continue;

				const AABB& box = obj->aabb;

				if (!Collision_IsOverlapAABB(box, playerAabb)) continue;
				//DumpLogAABB("Player", playerAabb);
				//DumpLogAABB("Cube  ", box);


				// 各軸の食い込み量（overlap）
				const float ox = std::min(playerAabb.max.x, box.max.x) - std::max(playerAabb.min.x, box.min.x);
				const float oy = std::min(playerAabb.max.y, box.max.y) - std::max(playerAabb.min.y, box.min.y);
				const float oz = std::min(playerAabb.max.z, box.max.z) - std::max(playerAabb.min.z, box.min.z);

				if (ox <= 0 || oy <= 0 || oz <= 0) continue;

				// 中心で押し戻し方向を決める
				const float pcx = (playerAabb.min.x + playerAabb.max.x) * 0.5f;
				const float pcy = (playerAabb.min.y + playerAabb.max.y) * 0.5f;
				const float pcz = (playerAabb.min.z + playerAabb.max.z) * 0.5f;

				const float bcx = (box.min.x + box.max.x) * 0.5f;
				const float bcy = (box.min.y + box.max.y) * 0.5f;
				const float bcz = (box.min.z + box.max.z) * 0.5f;

				// 最小の食い込み軸で押し戻す（MTV）
				if (ox <= oy && ox <= oz)
				{
					const float dir = (pcx < bcx) ? -1.0f : +1.0f;
					position = XMVectorSetX(position, XMVectorGetX(position) + dir * ox);

					// 壁に当たったらX速度を止める
					velocity = XMVectorSetX(velocity, 0.0f);
				}
				else if (oy <= ox && oy <= oz)
				{
					const float dir = (pcy < bcy) ? -1.0f : +1.0f;
					position = XMVectorSetY(position, XMVectorGetY(position) + dir * oy);

					// 上から着地（押し戻しが +Y）ならジャンプ終了
					if (dir > 0.0f)
					{
						g_isGrounded = false;
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

			if (!anyHit) break;
		}
	}
	*/

	// ===== Build wall/ledge sensors (post-resolve) =====
// ★ここだけあなたの実装に合わせて置換
	/*
	AABB  playerAabb = Player_ConvertPositionToAABB(position);
	float velY = XMVectorGetY(velocity);
	bool  onGround = g_isGrounded;

	PlayerWallLedgeSensors s{};
	PlayerSensors_BuildWallLedge(playerAabb, velY, onGround, g_ledgeTune, s);

	g_isWallTouch = s.wallTouch;
	g_wallNormal = s.wallNormal;

	g_isLedgeAvailable = s.ledgeAvailable;
	g_ledgeHangPos = s.ledgeHangPos;
	g_ledgeNormal = s.ledgeNormal;
	*/

		
		
		/*if (Collision_IsOverlapAABB(cube, player)) { // 上にたぶんのっかった
			if (XMVectorGetY(velocity) < 0.0f) {
				position -= velocity * (float)elapsedTime;
				//gvelocity = {};
				velocity *= { 1.0f, 0.0f, 1.0f };
				g_isJump = false;
			}
		}*/

		/*
		for (int i = 0; i < Map_GetObjectsCount(); i++) 
		{
			AABB player = Player_ConvertPositionToAABB(position);

			//ここが色々なオブジェクトに対応できるように
			AABB object = Map_GetObject(i)->aabb;
			Hit hit = Collision_IsHitAABB(object, player);
			if (hit.isHit) {
				if (hit.normal.y > 0.0f) {
					//position -= gvelocity;
					position = XMVectorSetY(position, object.max.y); // 戻り値を代入する //XMVectorSetY(position, cube.max.y);
					//gvelocity={};
					velocity *= { 1.0f, 0.0f, 1.0f };
					g_isJump = false;
				}
			}
			/*else if (XMVectorGetY(position) < 0.0f) {
				position -= gvelocity;
				//gvelocity={};
				velocity *= { 1.0f, 0.0f, 1.0f };
				g_isJump = false;
			}
		}*/

		
/*
		XMStoreFloat3(&g_playerPos, position);
		XMStoreFloat3(&g_playerVel, velocity);
		*/

		//壁ずり
		// 
		//移動したあとのプレイヤーとマップオブジェクトとの当たり判定
		/*
		for (int i = 0; i < Map_GetObjectsCount(); i++)
		{
			AABB player = Player_ConvertPositionToAABB(position);

			//ここが色々なオブジェクトに対応できるように
			AABB object = Map_GetObject(i)->aabb;;
			Hit hit = Collision_IsHitAABB(object, player);

			if (hit.isHit) {
				if (hit.normal.x > 0.0f) {
					position = XMVectorSetX(position, object.max.x + PLAYER_HALF_WIDTH);
					velocity *= {0.0f, 1.0f, 1.0f};
				}
				else if (hit.normal.x < 0.0f) {
					position = XMVectorSetX(position, object.min.x - PLAYER_HALF_WIDTH);
					velocity *= {0.0f, 1.0f, 1.0f};
				}
				else if (hit.normal.y > 0.0f) {
					position = XMVectorSetY(position, object.max.y);
					velocity *= {1.0f, 0.0f, 1.0f};
				}
				else if (hit.normal.y < 0.0f) {
					position = XMVectorSetY(position, object.min.y - PLAYER_HEIGHT);
					velocity *= {1.0f, 0.0f, 1.0f};
				}
				else if (hit.normal.z > 0.0f) {
					position = XMVectorSetZ(position, object.max.z + PLAYER_HALF_DEPTH);
					velocity *= {1.0f, 1.0f, 0.0f};
				}
				else if (hit.normal.z < 0.0f) {
					position = XMVectorSetZ(position, object.min.z - PLAYER_HALF_DEPTH);
					velocity *= {1.0f, 1.0f, 0.0f};
				}
			}
		}*/


		/*
		XMStoreFloat3(&g_playerPos, position);
		XMStoreFloat3(&g_playerVel, velocity);

		static float t = 0.0f;
        t += (float)elapsedTime;
        SkinnedModel_Update(g_playerModel, t, 21);//n番アニメを再生
}
*/

void Player_Draw()
{
	Light_SetSpecularWorld(Camera_GetPosition(), 4.0f, { 0.2f,0.2f,0.2f,1.0f });

	float angleX = 90.0f;
	float angleY = 0.0f;
	
	//-atan2f(g_playerFront.z, g_playerFront.x) + XMConvertToRadians(270);
	//XMMATRIX r = XMMatrixRotationX(XMConvertToRadians(angleX)) * XMMatrixRotationY(XMConvertToRadians(angleY));
	float angle = -atan2f(g_playerFront.z, g_playerFront.x) + XMConvertToRadians(270);
	XMMATRIX r = XMMatrixRotationX(XMConvertToRadians(angleX))*XMMatrixRotationY(angle);

	// ===== Visual-only fixed offset =====
	float fix = 0.0f;
	if (g_visFixLand)      fix = g_visLandForwardFix;
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

	XMMATRIX r = XMMatrixRotationY(angle);
	// ===== Visual-only fixed offset =====
	float fix = 0.0f;
	if (g_visFixLand)      fix = g_visLandForwardFix;
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

PlayerLedgeTuning* Player_GetLedgeTuning()
{
	return &g_ledgeTune;
}
