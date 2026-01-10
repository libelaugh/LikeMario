/*==============================================================================

　　  アクション状態管理[player_action.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/07
--------------------------------------------------------------------------------

==============================================================================*/
#ifndef PLAYER_ACTION_H
#define PLAYER_ACTION_H

#include <DirectXMath.h>

// What the player is currently doing (high-level).
enum class PlayerActionId
{
    Ground,
    Air,
    Spin,
    Crouch,
    //WallGrab,
    //LedgeHang,
};

// Converted input for the action system.
struct PlayerActionInput
{
    float moveX = 0.0f; // -1..+1
    float moveY = 0.0f; // -1..+1

    bool  jumpHeld = false;
    bool  dashHeld = false;
    bool  spinHeld = false;
    bool  crouchHeld = false;
};


// Environment/contact information the action system needs.
struct PlayerActionSensors
{
    bool onGround = false;
    float velY = 0.0f; // current vertical velocity (falling checks)
};

// Tuning parameters (wire these to ImGui later).
struct PlayerActionParams
{
    // Spin
    float spinTime = 0.50f;               // 発動時間
    float spinCooldown = 0.80f;           // クールタイム
    float spinAirDelayAfterJump = 0.20f;  // ジャンプ直後はこの時間だけ空中スピン不可
    float spinAirLiftImpulseY = 4.5f;   // 合計でどれだけ上向きΔVを足すか
    float spinAirLiftDuration = 0.5f;


    // Crouch
    float crouchSpeedScale = 0.45f;


    // Jump (action-level only; player.cpp applies to physics)
    float jumpSpeedY = 7.0f;
};

// Internal state for action system.
struct PlayerActionState
{
    PlayerActionId id = PlayerActionId::Ground;

    float timer = 0.0f;            // per-state timer

    // Spin timers
    float spinCooldownTimer = 0.0f; // スピンクールタイム残り
    float spinAirDelayTimer = 0.0f; // ジャンプ直後の空中スピン不可残り


    // Edge detection
    bool prevJump = false;
    bool prevSpin = false;
    bool prevCrouch = false;

    // Remember state before spin (optional)
    PlayerActionId prevBeforeSpin = PlayerActionId::Ground;
};

// Output of action system. player.cpp applies these to its physics variables.
struct PlayerActionOutput
{
    PlayerActionId id = PlayerActionId::Ground;

    // Movement modifiers
    float moveSpeedScale = 1.0f; // multiply your current ground/air speed

    // Jump request
    bool  requestJump = false;
    float jumpSpeedY = 0.0f;
    
    // Spin（空中で上向きに速度を加算）
    bool  addVelY = false;
    float velYDelta = 0.0f;

    // Gravity/vertical control
    bool  overrideVelY = false;
    float velY = 0.0f;

    // Full velocity override (e.g. wall jump)
    bool  overrideVelocity = false;
    DirectX::XMFLOAT3 velocity{ 0,0,0 };

};

void PlayerAction_Init(PlayerActionState& st);
void PlayerAction_InitDefaultParams(PlayerActionParams& p);

void PlayerAction_Update(PlayerActionState& st,
    const PlayerActionParams& p,
    const PlayerActionInput& in,
    const PlayerActionSensors& s,
    float dt,
    PlayerActionOutput& out);


#endif//PLAYER_ACTION_H