/*==============================================================================

　　　タイトルシーン[title.cpp]
														 Author : Tanaka Kouki
														 Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#include "title.h"
#include "direct3d.h"
#include "gamepad.h"
#include "key_logger.h"
#include "sprite.h"
#include "stage_registry.h"
#include "texture.h"
#include "debug_text.h"
#include"Audio.h"

static int space = -1;
static int titleBgm = -1;

namespace
{
    enum class TitleState
    {
        Logo,
        StageSelect,
    };

    constexpr int kStageIconCount = 3;
    constexpr float kStickThreshold = 0.5f;
    constexpr float kIconBaseSize = 330.0f;
    constexpr float kIconSelectedSize = 450.0f;
    constexpr float kIconSpacing = 170.0f;
    static float angle{};

    const wchar_t kTitleLogoPath[] = L"title_logo.png";
    const wchar_t kStageIconPath[] = L"stage_icons.png";


    struct IconRect
    {
        int x;
        int y;
        int w;
        int h;
    };

    constexpr IconRect kStageIconRects[kStageIconCount] = {
        {1134, 546, 1700 - 1134, 1092 - 546},
        {0, 546, 567 - 0, 1092 - 546},
        {567, 546, 1134 - 567, 1092 - 546},
    };

    int g_titleLogoTex = -1;
    int g_stageIconTex = -1;
    hal::DebugText* g_titleText = nullptr;
    TitleState g_state = TitleState::Logo;
    int g_selectedStage = 0;

    bool g_finished = false;

    bool g_prevPadA = false;
    bool g_prevPadB = false;
    bool g_prevStickLeft = false;
    bool g_prevStickRight = false;

    bool IsTrigger(bool current, bool* prev)
    {
        if (!prev) return false;
        const bool triggered = current && !(*prev);
        *prev = current;
        return triggered;
    }

    void DrawCenteredLogo()
    {
        if (g_titleLogoTex < 0)
            return;

        const float screenW = (float)Direct3D_GetBackBufferWidth();
        const float screenH = (float)Direct3D_GetBackBufferHeight();
        const float w = (float)Texture_Width(g_titleLogoTex);
        const float h = (float)Texture_Height(g_titleLogoTex);
        const float x = (screenW - w) * 0.5f;
        const float y = (screenH - h) * 0.5f+50.0f;
        Sprite_Draw(g_titleLogoTex, x, y);
    }

    void DrawStageIcons()
    {
        if (g_stageIconTex < 0)
            return;

        const float screenW = (float)Direct3D_GetBackBufferWidth();
        const float screenH = (float)Direct3D_GetBackBufferHeight();

        const float baseTotal = kIconBaseSize * kStageIconCount + kIconSpacing * (kStageIconCount - 1);
        const float startX = (screenW - baseTotal) * 0.5f;
        const float baseY = screenH * 0.6f;

        for (int i = 0; i < kStageIconCount; ++i)
        {
            const bool selected = (i == g_selectedStage);
            const float drawSize = selected ? kIconSelectedSize : kIconBaseSize;
            const float offsetX = selected ? 40.0f : 0.0f;
            const float x = startX + i * (kIconBaseSize + kIconSpacing) + (kIconBaseSize - drawSize) * 0.5f + 140.0f + offsetX;
            const float y = baseY + (kIconBaseSize - drawSize) * 0.5f + -40.0f;
            const IconRect& rect = kStageIconRects[i];
            Sprite_Draw(g_stageIconTex, x, y, drawSize, drawSize, rect.x, rect.y, rect.w, rect.h, angle);
        }
    }
}

void Title_Initialize()
{
    g_titleLogoTex = Texture_Load(kTitleLogoPath);
    g_stageIconTex = Texture_Load(kStageIconPath);
    g_state = TitleState::Logo;
    g_selectedStage = 0;
    g_prevPadA = false;
    g_prevPadB = false;
    g_prevStickLeft = false;
    g_prevStickRight = false;
    g_finished = false;

    g_state = TitleState::Logo;
    g_finished = false;
    g_selectedStage = 0;

    titleBgm = LoadAudio("title.wav");
    space = Texture_Load(L"space1.png");

    const float screenW = (float)Direct3D_GetBackBufferWidth();
    const float screenH = (float)Direct3D_GetBackBufferHeight();
    const float textOffsetX = screenW * 0.5f - 230.0f;
    const float textOffsetY = screenH * 0.8f + 100;

    g_titleText = new hal::DebugText(
        Direct3D_GetDevice(),
        Direct3D_GetContext(),
        L"consolab_ascii_512.png",
        (UINT)screenW,
        (UINT)screenH,
        textOffsetX,
        textOffsetY);

    PlayAudio(titleBgm, true);
}

void Title_Finalize()
{
    delete g_titleText;
    g_titleText = nullptr;
    UnloadAudio(titleBgm);
}

void Title_Update(double elapsed_time)
{
    
    angle += 1.0f * (float)elapsed_time;

    GamepadState pad{};
    const bool padConnected = Gamepad_GetState(0, &pad);

    const bool padBTrigger = padConnected && IsTrigger(pad.b, &g_prevPadB);

    const bool stickLeft = padConnected && (pad.lx < -kStickThreshold);
    const bool stickRight = padConnected && (pad.lx > kStickThreshold);
    const bool stickLeftTrigger = padConnected && IsTrigger(stickLeft, &g_prevStickLeft);
    const bool stickRightTrigger = padConnected && IsTrigger(stickRight, &g_prevStickRight);

    //パッドが繋がってる間はキーボード無効
    const bool allowKeyboard = !padConnected;

    const bool enterTrigger = allowKeyboard && KeyLogger_IsTrigger(KK_ENTER);

    static bool s_waitRelease = false;

    // ---- Logo ----
    if (g_state == TitleState::Logo)
    {
        if (padBTrigger || enterTrigger)
        {
            g_state = TitleState::StageSelect;
            s_waitRelease = true;
        }
        return;
    }

    // ---- StageSelect:
    const bool enterHeld = allowKeyboard && KeyLogger_IsPressed(KK_ENTER);
    const bool bHeld = padConnected && pad.b;

    if (s_waitRelease)
    {
        if (!enterHeld && !bHeld)
        {
            s_waitRelease = false; // 離したら操作解禁
        }
        return;
    }

    // ---- StageSelect: 左右で選択 ----
    static bool s_prevA = false;
    static bool s_prevD = false;

    const bool aHeld = allowKeyboard && KeyLogger_IsPressed(KK_A);
    const bool dHeld = allowKeyboard && KeyLogger_IsPressed(KK_D);

    const bool aTrigger = aHeld && !s_prevA;
    const bool dTrigger = dHeld && !s_prevD;

    // ここで選択
    if (stickLeftTrigger || aTrigger)
    {
        g_selectedStage = (g_selectedStage + kStageIconCount - 1) % kStageIconCount;
    }
    else if (stickRightTrigger || dTrigger)
    {
        g_selectedStage = (g_selectedStage + 1) % kStageIconCount;
    }

    s_prevA = aHeld;
    s_prevD = dHeld;


    // ---- StageSelect: 決定 ----
    if (padBTrigger || enterTrigger)
    {
        UnloadAudio(titleBgm);
        g_finished = true;
    }
}


void Title_Draw()
{
    Direct3D_SetDepthEnable(false);
    Sprite_Begin();

    if (g_state == TitleState::Logo)
    {
        const float screenW = (float)Direct3D_GetBackBufferWidth();
        const float screenH = (float)Direct3D_GetBackBufferHeight();
        Sprite_Draw(space, 0, 0, screenW, screenH);
        DrawCenteredLogo();
        Direct3D_SetDepthEnable(true);
    }
    else
    {
        const float screenW = (float)Direct3D_GetBackBufferWidth();
        const float screenH = (float)Direct3D_GetBackBufferHeight();
        Sprite_Draw(space, 0, 0, screenW, screenH);
        DrawStageIcons();
        Direct3D_SetDepthEnable(true);
    }

    if (g_titleText)
    {
        if (g_state == TitleState::Logo)
        {
            g_titleText->SetText("Press B or Enter", { 1.0f, 1.0f, 1.0f, 1.0f });
        }
        else
        {
            g_titleText->SetText("Select Stage!!", { 1.0f, 1.0f, 1.0f, 1.0f });
        }
        g_titleText->Draw();
        g_titleText->Clear();
        Direct3D_SetDepthEnable(true);
    }
}

bool Title_IsFinished()
{
    return g_finished;
}

StageId Title_GetSelectedStage()
{
    return static_cast<StageId>(g_selectedStage);
}