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
    WallGrab,
    LedgeHang,
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

    // Wall touch (vertical face contact)
    bool wallTouch = false;
    DirectX::XMFLOAT3 wallNormal{ 0,0,0 }; // wall -> player (unit axis is ideal)

    // Ledge candidate (optional)
    bool ledgeAvailable = false;
    DirectX::XMFLOAT3 ledgeHangPos{ 0,0,0 }; // desired snap position (center)
    DirectX::XMFLOAT3 ledgeNormal{ 0,0,0 };  // ledge/wall outward (wall -> player)
};

// Tuning parameters (wire these to ImGui later).
struct PlayerActionParams
{
    // Spin
    float spinTime = 0.40f;

    // Crouch
    float crouchSpeedScale = 0.45f;

    // Wall grab / wall jump
    float wallGrabSlideSpeed = -1.5f; // y velocity while wall grabbing (negative = slide down)
    float wallJumpUpSpeed = 6.5f;
    float wallJumpAwaySpeed = 5.0f;   // push away from wall normal

    // Ledge hang
    float ledgeDropDelay = 0.10f;     // prevent immediate re-grab after drop

    // Jump (action-level only; player.cpp applies to physics)
    float jumpSpeedYWeak = 5.0f;
    float jumpSpeedYMid = 6.0f;
    float jumpSpeedYStrong = 7.0f;
    float jumpHoldMidTime = 0.10f;
    float jumpHoldStrongTime = 0.20f;
};

// Internal state for action system.
struct PlayerActionState
{
    PlayerActionId id = PlayerActionId::Ground;

    float timer = 0.0f;            // per-state timer
    float noLedgeGrabTimer = 0.0f; // cooldown after ledge drop

    // Edge detection
    bool prevJump = false;
    bool prevSpin = false;
    bool prevCrouch = false;

    // Remember state before spin (optional)
    PlayerActionId prevBeforeSpin = PlayerActionId::Ground;

    // Jump charge
    float jumpHoldTime = 0.0f;
};

// Output of action system. player.cpp applies these to its physics variables.
struct PlayerActionOutput
{
    PlayerActionId id = PlayerActionId::Ground;

    // Movement modifiers
    float moveSpeedScale = 1.0f; // multiply your current ground/air speed
    bool  lockMoveXZ = false;    // if true: ignore input movement this frame

    // Jump request
    bool  requestJump = false;
    float jumpSpeedY = 0.0f;

    // Gravity/vertical control
    bool  overrideVelY = false;
    float velY = 0.0f;

    // Full velocity override (e.g. wall jump)
    bool  overrideVelocity = false;
    DirectX::XMFLOAT3 velocity{ 0,0,0 };

    // Position snap (ledge hang)
    bool  overridePosition = false;
    DirectX::XMFLOAT3 position{ 0,0,0 };
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