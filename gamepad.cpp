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
#include <dinput.h>
#include <algorithm>
#include <cmath>

#pragma comment(lib, "xinput.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace
{
    // --------------------------
    // XInput
    // --------------------------
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
        f = (f < 0.0f) ? 0.0f :
            (f > 1.0f) ? 1.0f :
            f;
        return f * (float)sign;
    }

    float NormalizeTriggerXI(BYTE v, BYTE threshold)
    {
        if (v <= threshold) return 0.0f;
        float f = (v - (float)threshold) / (255.0f - (float)threshold);
        f = (f < 0.0f) ? 0.0f :
            (f > 1.0f) ? 1.0f :
            f;
        return f;
    }

    bool AnyXInputConnected()
    {
        for (int i = 0; i < 4; ++i) if (g_xiConnected[i]) return true;
        return false;
    }

    // --------------------------
    // DirectInput (fallback)
    // --------------------------
    IDirectInput8* g_di = nullptr;
    IDirectInputDevice8* g_joy = nullptr;
    bool g_diTriedInit = false;
    bool g_diConnected = false;
    DIJOYSTATE2 g_js{};

    float NormalizeAxisDI(LONG v, LONG deadzone) // v: -32768..32767
    {
        const int iv = (int)v;
        const int sign = (iv < 0) ? -1 : 1;
        int av = std::abs(iv);

        if (av <= (int)deadzone) return 0.0f;

        const float maxv = 32767.0f;
        float f = (av - (float)deadzone) / (maxv - (float)deadzone);
        f = (f < 0.0f) ? 0.0f :
            (f > 1.0f) ? 1.0f :
            f;
        return f * (float)sign;
    }

    struct EnumCtx
    {
        IDirectInput8* di = nullptr;
    };

    BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
    {
        auto* ctx = (EnumCtx*)pContext;
        if (!ctx || !ctx->di) return DIENUM_CONTINUE;

        // 1台だけ使う（最初に見つかったやつ）
        if (FAILED(ctx->di->CreateDevice(pdidInstance->guidInstance, &g_joy, nullptr)))
            return DIENUM_CONTINUE;

        return DIENUM_STOP;
    }

    void SetAxisRange(IDirectInputDevice8* dev, DWORD objOffset, LONG minV, LONG maxV)
    {
        DIPROPRANGE diprg{};
        diprg.diph.dwSize = sizeof(DIPROPRANGE);
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        diprg.diph.dwHow = DIPH_BYOFFSET;
        diprg.diph.dwObj = objOffset;
        diprg.lMin = minV;
        diprg.lMax = maxV;
        dev->SetProperty(DIPROP_RANGE, &diprg.diph);
    }

    void TryInitDirectInput()
    {
        if (g_diTriedInit) return;
        g_diTriedInit = true;

        HRESULT hr = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION,
            IID_IDirectInput8, (void**)&g_di, nullptr);
        if (FAILED(hr) || !g_di) return;

        EnumCtx ctx{};
        ctx.di = g_di;

        // ゲームコントローラを列挙
        hr = g_di->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, &ctx, DIEDFL_ATTACHEDONLY);
        if (FAILED(hr) || !g_joy) return;

        // データ形式
        g_joy->SetDataFormat(&c_dfDIJoystick2);

        // Cooperative（ウィンドウハンドルが必要）
        HWND hWnd = GetForegroundWindow();
        if (!hWnd) hWnd = GetActiveWindow();
        if (!hWnd) hWnd = GetDesktopWindow();

        g_joy->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);

        // 軸レンジを揃える（-32768..32767）
        SetAxisRange(g_joy, DIJOFS_X, -32768, 32767);
        SetAxisRange(g_joy, DIJOFS_Y, -32768, 32767);
        SetAxisRange(g_joy, DIJOFS_Z, -32768, 32767);
        SetAxisRange(g_joy, DIJOFS_RX, -32768, 32767);
        SetAxisRange(g_joy, DIJOFS_RY, -32768, 32767);
        SetAxisRange(g_joy, DIJOFS_RZ, -32768, 32767);

        g_joy->Acquire();
    }

    bool PollDirectInput()
    {
        if (!g_joy) return false;

        HRESULT hr = g_joy->Poll();
        if (FAILED(hr))
        {
            // フォーカス喪失などで未取得になるので取り直す
            hr = g_joy->Acquire();
            while (hr == DIERR_INPUTLOST) hr = g_joy->Acquire();
            if (FAILED(hr)) return false;

            hr = g_joy->Poll();
            if (FAILED(hr)) return false;
        }

        hr = g_joy->GetDeviceState(sizeof(DIJOYSTATE2), &g_js);
        if (FAILED(hr)) return false;

        return true;
    }
}

// ---------------------------------------------------------
// Public API
// ---------------------------------------------------------
void Gamepad_Update()
{
    // 1) XInput更新
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

    // 2) XInputが1台も無いならDirectInputを試す
    if (!AnyXInputConnected())
    {
        TryInitDirectInput();
        g_diConnected = PollDirectInput();
    }
    else
    {
        g_diConnected = false;
    }
}

bool Gamepad_GetState(int idx, GamepadState* out)
{
    if (!out) return false;
    *out = GamepadState{};

    // XInputがあるならXInput優先
    if (AnyXInputConnected())
    {
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

    // それ以外はDirectInput（とりあえず idx=0 のみ対応）
    if (idx != 0) return false;
    if (!g_diConnected) return false;

    // ---- Axis selection (左スティック候補を自動選択) ----
    auto absL = [](LONG v) { return (v < 0) ? -v : v; };

    struct Pair { LONG x; LONG y; };
    Pair cands[] = {
        { g_js.lX,  g_js.lY  },
        { g_js.lRx, g_js.lRy },
        { g_js.lZ,  g_js.lRz },
    };

    static int  s_pair = -1;
    if (s_pair < 0)
    {
        // 初回は「一番中心っぽい（|x|+|y|が小さい）」ペアを採用
        int best = 0;
        LONG bestScore = absL(cands[0].x) + absL(cands[0].y);
        for (int i = 1; i < (int)(sizeof(cands) / sizeof(cands[0])); ++i)
        {
            LONG score = absL(cands[i].x) + absL(cands[i].y);
            if (score < bestScore) { bestScore = score; best = i; }
        }
        s_pair = best;
    }

    LONG rawX = cands[s_pair].x;
    LONG rawY = cands[s_pair].y;

    // ---- Center calibration (最初の約0.5秒で中心値を取る) ----
    static bool s_calibDone = false;
    static int  s_calibFrames = 0;
    static long long s_sumX = 0, s_sumY = 0;
    static LONG s_centerX = 0, s_centerY = 0;

    constexpr int CALIB_FRAME_COUNT = 30; // 60fps想定で約0.5秒

    if (!s_calibDone)
    {
        // ※ここだけ：起動直後はスティック触らないでね（中心を覚える）
        s_sumX += rawX;
        s_sumY += rawY;
        s_calibFrames++;

        if (s_calibFrames >= CALIB_FRAME_COUNT)
        {
            s_centerX = (LONG)(s_sumX / s_calibFrames);
            s_centerY = (LONG)(s_sumY / s_calibFrames);
            s_calibDone = true;
        }

        // キャリブ中は入力0扱い
        GamepadState s{};
        s.connected = true;
        *out = s;
        return true;
    }

    LONG dx = rawX - s_centerX;
    LONG dy = rawY - s_centerY;

    // ---- Normalize centered axis (-1..1) with deadzone ----
    auto NormalizeCentered = [](LONG dv, LONG deadzone) -> float
        {
            const int sign = (dv < 0) ? -1 : +1;
            int av = abs(dv);

            if (av <= (int)deadzone) return 0.0f;

            const float maxv = 32767.0f; // 差分はだいたいこのレンジに収まる
            float f = (av - (float)deadzone) / (maxv - (float)deadzone);
            f = (f < 0.0f) ? 0.0f :
                (f > 1.0f) ? 1.0f :
                f;
            return f * (float)sign;
        };

    constexpr LONG DEADZONE = 8000;  // ドリフト強めなので少し大きめ
    constexpr float SMOOTH = 0.35f;  // 暴れ防止（0.25〜0.45で調整）

    float lxRaw = NormalizeCentered(dx, DEADZONE);
    float lyRaw = -NormalizeCentered(dy, DEADZONE); // Y反転してXInputと合わせる

    static float s_lx = 0.0f;
    static float s_ly = 0.0f;
    s_lx += (lxRaw - s_lx) * SMOOTH;
    s_ly += (lyRaw - s_ly) * SMOOTH;

    GamepadState s{};
    s.connected = true;
    s.lx = s_lx;
    s.ly = s_ly;

    // ボタン（移動だけなら不要だけど一応）
    auto btn = [&](int i) { return (g_js.rgbButtons[i] & 0x80) != 0; };
    s.a = btn(0);
    s.b = btn(1);
    s.x = btn(2);
    s.y = btn(3);
    s.lb = btn(4);
    s.rb = btn(5);
    s.back = btn(6);
    s.start = btn(7);

    s.lt = 0.0f;
    s.rt = 0.0f;

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