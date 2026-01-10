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
    if (st.spinCooldownTimer > 0.0f)
        st.spinCooldownTimer = std::max(0.0f, st.spinCooldownTimer - dt);
    if (st.spinAirDelayTimer > 0.0f)
        st.spinAirDelayTimer = std::max(0.0f, st.spinAirDelayTimer - dt);

    // 「このフレームでスピンに入ったか」
    bool startedSpinThisFrame = false;



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


    switch (st.id)
    {
    case PlayerActionId::Ground:
    {
        if (spinTrg && st.spinCooldownTimer <= 0.0f)
        {
            startedSpinThisFrame = true;
            st.prevBeforeSpin = PlayerActionId::Ground;
            ChangeState(PlayerActionId::Spin);
        }

        else if (in.crouchHeld && s.onGround)
        {
            ChangeState(PlayerActionId::Crouch);
        }
        else if (jumpTrg)
        {
            out.requestJump = true;
            out.jumpSpeedY = p.jumpSpeedY;
            st.spinAirDelayTimer = p.spinAirDelayAfterJump;
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
        if (spinTrg && st.spinCooldownTimer <= 0.0f && st.spinAirDelayTimer <= 0.0f)
        {
            startedSpinThisFrame = true;
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
        out.moveSpeedScale = 0.90f;

        // 空中スピン中の最初の少しだけ上向き加速（上昇中でも落下中でも効く）
        // 空中スピン中：上昇中に発動したら上昇速度を0にし、スピン中は重力を切る（overrideVelY）
        if (st.prevBeforeSpin == PlayerActionId::Air && !s.onGround)
        {
            float vy = s.velY;

            // 上昇中でも落下中でも開始時に縦速度を止める
            if (startedSpinThisFrame)
                vy = 0.0f;

            // 最初の少しだけ上向き加速（合計 impulse になる）
            if (st.timer <= p.spinAirLiftDuration)
            {
                const float accelY = (p.spinAirLiftImpulseY / std::max(1.0e-6f, p.spinAirLiftDuration));
                vy += accelY * dt;
            }

            // player.cpp 側が overrideVelY=true のとき重力を適用しない前提
            out.overrideVelY = true;
            out.velY = vy;
        }


        if (st.timer >= p.spinTime)
        {
            st.spinCooldownTimer = p.spinCooldown;
            ChangeState(s.onGround ? PlayerActionId::Ground : PlayerActionId::Air);
        }
        break;
    }



    case PlayerActionId::Crouch:
    {
        out.moveSpeedScale = 0.0f;

        if (jumpTrg)
        {
            out.requestJump = true;
            out.jumpSpeedY = p.jumpSpeedY;
            st.spinAirDelayTimer = p.spinAirDelayAfterJump;
            ChangeState(PlayerActionId::Air);
        }

        if (!in.crouchHeld)
            ChangeState(s.onGround ? PlayerActionId::Ground : PlayerActionId::Air);

        if (!s.onGround)
            ChangeState(PlayerActionId::Air);

        break;
    }
    default:
        ChangeState(s.onGround ? PlayerActionId::Ground : PlayerActionId::Air);
        break;
    }

    // ※遷移したそのフレーム用：Spin case に入らないのでここで空中スピンの縦制御を1回当てる
    if (startedSpinThisFrame && st.id == PlayerActionId::Spin &&
        st.prevBeforeSpin == PlayerActionId::Air && !s.onGround)
    {
        float vy = s.velY;
        if (vy > 0.0f) vy = 0.0f;

        const float accelY = (p.spinAirLiftImpulseY / std::max(1.0e-6f, p.spinAirLiftDuration));
        vy += accelY * dt;

        out.overrideVelY = true;
        out.velY = vy;
    }


    out.id = st.id;
}

