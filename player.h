/*==============================================================================

	プレイヤー制御[player.h]
														 Author : Tanaka Kouki
														 Date   : 2025/10/31
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef PLAYER_H
#define PLAYER_H

#include"collision.h"
#include "player_action.h"
#include "stage01_manage.h"
#include<DirectXMath.h>

struct PlayerInput
{
	float moveX = 0.0f;   // 左(-1) 右(+1)
	float moveY = 0.0f;   // 後(-1) 前(+1)
	bool  jump = false;
	bool  dash = false;
	bool  spin = false;
	bool  crouch = false;

};

// MotionLab から入力を上書きする
void Player_SetInputOverride(bool enable, const PlayerInput* input); // input=nullptrならニュートラル
bool Player_IsInputOverrideEnabled();

// MotionLab用：同じ条件で何度も測れるように
void Player_DebugTeleport(const DirectX::XMFLOAT3& pos, bool resetVelocity);

// MotionLab用：着地検出（これがあるとジャンプ計測が超安定）
bool Player_IsGrounded();

void Player_Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front);
void Player_Finalize();
void Player_Update(double elapsedTime);
void Player_Draw();
void Player_DepthDraw();

const DirectX::XMFLOAT3& Player_GetPosition();
const DirectX::XMFLOAT3& Player_GetFront();
AABB Player_GetAABB();

AABB Player_ConvertPositionToAABB(const DirectX::XMVECTOR& position);

struct PlayerTuning
{
	float jumpImpulse;
	float gravity;
	float terminalFall; // 終端速度
	float moveAccel;
	float friction;
	float rotSpeed;
};

PlayerTuning* Player_GetTuning();
const PlayerTuning& Player_GetDefaultTuning();
void Player_ResetTuning();

// forward declare（player_sensors.h の型）
struct PlayerLedgeTuning;

// MotionLab / EditorUI 用
const DirectX::XMFLOAT3& Player_GetVelocity();
PlayerActionParams* Player_GetActionParams();
PlayerActionState* Player_GetActionState();

DirectX::XMFLOAT3 Player_GetDestroyedBrickBlockPosition(StageBlock*o);



#endif //PLAYER_H