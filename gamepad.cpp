/*==============================================================================

　　  ゲームパッド入出力[gamepad.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/04
--------------------------------------------------------------------------------

==============================================================================*/
#include "gamepad.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Xinput.h>
#include <cmath>

#pragma comment(lib, "xinput.lib")

namespace
{
    XINPUT_STATE g_xiState[4]{};
    bool g_xiConnected[4]{};

    float NormalizeStickXI(short v, short deadzone)
    {
        const int iv = (int)v;
        const int sign = (iv < 0) ? -1 : 1;
        int av = std::abs(iv);

        if (av <= (int)deadzone) return 0.0f;

        const float maxv = 32767.0f;
        float f = (av - (float)deadzone) / (maxv - (float)deadzone);
        f = (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f;
        return f * (float)sign;
    }

    float NormalizeTriggerXI(BYTE v, BYTE threshold)
    {
        if (v <= threshold) return 0.0f;
        float f = (v - (float)threshold) / (255.0f - (float)threshold);
        f = (f < 0.0f) ? 0.0f : (f > 1.0f) ? 1.0f : f;
        return f;
    }
}

// ---------------------------------------------------------
// Public API
// ---------------------------------------------------------
void Gamepad_Update()
{
    // XInput更新
    for (int i = 0; i < 4; ++i)
    {
        XINPUT_STATE st{};
        DWORD r = XInputGetState((DWORD)i, &st);
        if (r == ERROR_SUCCESS)
        {
            g_xiState[i] = st;
            g_xiConnected[i] = true;
        }
        else
        {
            g_xiState[i] = XINPUT_STATE{};
            g_xiConnected[i] = false;
        }
    }
}

bool Gamepad_GetState(int idx, GamepadState* out)
{
    if (!out) return false;
    *out = GamepadState{};

    // XInputのみ対応
    if (idx < 0 || idx >= 4) return false;
    if (!g_xiConnected[idx]) return false;

    const auto& st = g_xiState[idx];
    const auto& g = st.Gamepad;

    const short stickDZ = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    const BYTE  trigTh = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    GamepadState s{};
    s.connected = true;

    s.lx = NormalizeStickXI(g.sThumbLX, stickDZ);
    s.ly = NormalizeStickXI(g.sThumbLY, stickDZ);

    s.lt = NormalizeTriggerXI(g.bLeftTrigger, trigTh);
    s.rt = NormalizeTriggerXI(g.bRightTrigger, trigTh);

    const WORD b = g.wButtons;
    s.a = (b & XINPUT_GAMEPAD_A) != 0;
    s.b = (b & XINPUT_GAMEPAD_B) != 0;
    s.x = (b & XINPUT_GAMEPAD_X) != 0;
    s.y = (b & XINPUT_GAMEPAD_Y) != 0;

    s.start = (b & XINPUT_GAMEPAD_START) != 0;
    s.back = (b & XINPUT_GAMEPAD_BACK) != 0;

    s.lb = (b & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
    s.rb = (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

    *out = s;
    return true;
}

#else

void Gamepad_Update() {}
bool Gamepad_GetState(int, GamepadState* out)
{
    if (out) *out = GamepadState{};
    return false;
}

#endif
