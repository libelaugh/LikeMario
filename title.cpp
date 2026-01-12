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

static int space = -1;

namespace
{
    enum class TitleState
    {
        Logo,
        StageSelect,
    };

    constexpr int kStageIconCount = 4;
    constexpr float kStickThreshold = 0.5f;
    constexpr float kIconBaseSize = 230.0f;
    constexpr float kIconSelectedSize = 320.0f;
    constexpr float kIconSpacing = 100.0f;
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
        {567, 0, 1134 - 567, 546 - 0},
        {567, 546, 1134 - 567, 1092 - 546},
    };

    int g_titleLogoTex = -1;
    int g_stageIconTex = -1;
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
        const float y = (screenH - h) * 0.5f;
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
            const float x = startX + i * (kIconBaseSize + kIconSpacing) + (kIconBaseSize - drawSize) * 0.5f+100.0f+offsetX;
            const float y = baseY + (kIconBaseSize - drawSize) * 0.5f + -40.0f;
            const IconRect& rect = kStageIconRects[i];
            Sprite_Draw(g_stageIconTex, x, y, drawSize, drawSize, rect.x, rect.y, rect.w, rect.h,angle);
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

    space = Texture_Load(L"space1.png");
}

void Title_Finalize()
{
}

void Title_Update(double elapsed_time)
{
    angle += 1.0f * (float)elapsed_time;

    GamepadState pad{};
    const bool padConnected = Gamepad_GetState(0, &pad);

    const bool padBTrigger = padConnected && IsTrigger(pad.b, &g_prevPadB);
    const bool padATrigger = padConnected && IsTrigger(pad.a, &g_prevPadA);

    const bool stickLeft = padConnected && (pad.lx < -kStickThreshold);
    const bool stickRight = padConnected && (pad.lx > kStickThreshold);
    const bool stickLeftTrigger = IsTrigger(stickLeft, &g_prevStickLeft);
    const bool stickRightTrigger = IsTrigger(stickRight, &g_prevStickRight);

    const bool enterTrigger = KeyLogger_IsTrigger(KK_ENTER);

    if (g_state == TitleState::Logo)
    {
        if (padBTrigger || enterTrigger)
        {
            g_state = TitleState::StageSelect;
        }
        return;
    }

    if (stickLeftTrigger || KeyLogger_IsTrigger(KK_W))
    {
        g_selectedStage = (g_selectedStage + kStageIconCount - 1) % kStageIconCount;
    }

    if (stickRightTrigger || KeyLogger_IsTrigger(KK_D))
    {
        g_selectedStage = (g_selectedStage + 1) % kStageIconCount;
    }

    if (padATrigger || enterTrigger)
    {
        g_finished = true;
    }
}

void Title_Draw()
{
    Sprite_Begin();

    if (g_state == TitleState::Logo)
    {
        DrawCenteredLogo();
        return;
    }
    const float screenW = (float)Direct3D_GetBackBufferWidth();
    const float screenH = (float)Direct3D_GetBackBufferHeight();
    Sprite_Draw(space, 0, 0, screenW, screenH);
    DrawStageIcons();
}

bool Title_IsFinished()
{
    return g_finished;
}

StageId Title_GetSelectedStage()
{
    return static_cast<StageId>(g_selectedStage);
}