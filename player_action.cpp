/*==============================================================================

　　  アクション状態管理[player_action.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/07
--------------------------------------------------------------------------------

==============================================================================*/
#include "player_action.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
    inline bool Trigger(bool now, bool prev) { return now && !prev; }

    inline XMFLOAT3 Mul(const XMFLOAT3& v, float s) { return { v.x * s, v.y * s, v.z * s }; }
    inline XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }

    inline float Clamp01(float v) {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    inline float InputMagnitude01(float x, float y)
    {
        const float m = std::sqrt(x * x + y * y);
        return Clamp01(m);
    }
}

void PlayerAction_Init(PlayerActionState& st)
{
    st = PlayerActionState{};
}

void PlayerAction_InitDefaultParams(PlayerActionParams& p)
{
    p = PlayerActionParams{};
}

void PlayerAction_Update(PlayerActionState& st,
    const PlayerActionParams& p,
    const PlayerActionInput& in,
    const PlayerActionSensors& s,
    float dt,
    PlayerActionOutput& out)
{
    // Timers
    st.timer += dt;
    if (st.noLedgeGrabTimer > 0.0f)
        st.noLedgeGrabTimer = std::max(0.0f, st.noLedgeGrabTimer - dt);

    // Triggers
    const bool jumpTrg = Trigger(in.jumpHeld, st.prevJump);
    const bool spinTrg = Trigger(in.spinHeld, st.prevSpin);
    const bool crouchTrg = Trigger(in.crouchHeld, st.prevCrouch);

    st.prevJump = in.jumpHeld;
    st.prevSpin = in.spinHeld;
    st.prevCrouch = in.crouchHeld;

    // Defaults
    out = PlayerActionOutput{};
    out.id = st.id;

    auto ChangeState = [&](PlayerActionId next)
        {
            st.id = next;
            st.timer = 0.0f;
            out.id = next;
        };

    const float moveMag = InputMagnitude01(in.moveX, in.moveY);
    const bool falling = (s.velY <= -0.01f);

    // Ledge grab (optional, highest priority)
    if (st.id != PlayerActionId::LedgeHang)
    {
        if (!s.onGround && falling && s.ledgeAvailable && st.noLedgeGrabTimer <= 0.0f)
        {
            ChangeState(PlayerActionId::LedgeHang);
        }
    }

    switch (st.id)
    {
    case PlayerActionId::Ground:
    {
        if (spinTrg)
        {
            st.prevBeforeSpin = PlayerActionId::Ground;
            ChangeState(PlayerActionId::Spin);
        }
        else if (in.crouchHeld)
        {
            ChangeState(PlayerActionId::Crouch);
        }
        else if (jumpTrg)
        {
            out.requestJump = true;
            out.jumpSpeedY = p.jumpSpeedY;
            ChangeState(PlayerActionId::Air);
        }
        else if (!s.onGround)
        {
            ChangeState(PlayerActionId::Air);
        }
        break;
    }

    case PlayerActionId::Air:
    {
        // Optional: wall grab trigger
        if (!s.onGround && falling && s.wallTouch && moveMag > 0.2f)
        {
            ChangeState(PlayerActionId::WallGrab);
            break;
        }

        if (spinTrg)
        {
            st.prevBeforeSpin = PlayerActionId::Air;
            ChangeState(PlayerActionId::Spin);
        }

        if (s.onGround)
        {
            ChangeState(PlayerActionId::Ground);
        }
        break;
    }

    case PlayerActionId::Spin:
    {
        // Slight slow-down is typical
        out.moveSpeedScale = 0.90f;

        if (st.timer >= p.spinTime)
        {
            ChangeState(s.onGround ? PlayerActionId::Ground : PlayerActionId::Air);
        }
        break;
    }

    case PlayerActionId::Crouch:
    {
        out.moveSpeedScale = p.crouchSpeedScale;

        if (jumpTrg)
        {
            out.requestJump = true;
            out.jumpSpeedY = p.jumpSpeedY;
            ChangeState(PlayerActionId::Air);
        }

        if (!in.crouchHeld)
            ChangeState(s.onGround ? PlayerActionId::Ground : PlayerActionId::Air);

        if (!s.onGround)
            ChangeState(PlayerActionId::Air);

        break;
    }

    case PlayerActionId::WallGrab:
    {
        out.lockMoveXZ = true;

        // Slide down with fixed speed
        out.overrideVelY = true;
        out.velY = p.wallGrabSlideSpeed;

        // Wall jump
        if (jumpTrg)
        {
            out.overrideVelocity = true;
            out.velocity = Add(Mul(s.wallNormal, p.wallJumpAwaySpeed),
                XMFLOAT3{ 0.0f, p.wallJumpUpSpeed, 0.0f });
            ChangeState(PlayerActionId::Air);
            break;
        }

        // Cancel conditions
        if (in.crouchHeld) ChangeState(PlayerActionId::Air);
        if (s.onGround)    ChangeState(PlayerActionId::Ground);
        if (!s.wallTouch)  ChangeState(PlayerActionId::Air);

        break;
    }

    case PlayerActionId::LedgeHang:
    {
        out.lockMoveXZ = true;

        // Stick the player at the hang point if provided
        out.overrideVelocity = true;
        out.velocity = { 0,0,0 };

        if (s.ledgeAvailable)
        {
            out.overridePosition = true;
            out.position = s.ledgeHangPos;
        }

        // Jump from ledge
        if (jumpTrg)
        {
            out.overrideVelocity = true;
            out.velocity = Add(Mul(s.ledgeNormal, p.wallJumpAwaySpeed * 0.6f),
                XMFLOAT3{ 0.0f, p.wallJumpUpSpeed, 0.0f });
            ChangeState(PlayerActionId::Air);
            break;
        }

        // Drop from ledge
        if (crouchTrg || in.crouchHeld)
        {
            st.noLedgeGrabTimer = p.ledgeDropDelay;
            ChangeState(PlayerActionId::Air);
            break;
        }

        if (s.onGround)
            ChangeState(PlayerActionId::Ground);

        break;
    }

    default:
        ChangeState(s.onGround ? PlayerActionId::Ground : PlayerActionId::Air);
        break;
    }

    out.id = st.id;
}

