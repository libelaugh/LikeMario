/*==============================================================================

	ƒS[ƒ‹[goal.cpp]
														 Author : Tanaka Kouki
														 Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#include "goal.h"

#include "player.h"
#include "collision.h"
#include "gamepad.h"
#include "key_logger.h"
#include "game.h"
#include "sprite.h"
#include "texture.h"
#include "direct3d.h"
#include "stage_registry.h"
#include"Audio.h"

#include "model.h"

using namespace DirectX;

static int goalBgm = -1;

namespace
{
    XMFLOAT3  g_goalPos = { 0,0,0 };
    GoalState g_goalState = GoalState::Inactive;

    MODEL* g_goalModel = nullptr;
    constexpr const char* kGoalModelPath = "goal.fbx";
    constexpr float       kGoalModelScale = 1.0f;
    constexpr bool        kGoalModelIsBrender = false;

    float g_goalYaw = 0.0f;

    bool LoadGoalModel()
    {
        if (g_goalModel) return true;
        g_goalModel = ModelLoad(kGoalModelPath, kGoalModelScale, kGoalModelIsBrender);
        return (g_goalModel != nullptr);
    }

    void ReleaseGoalModel()
    {
        if (!g_goalModel) return;
        ModelRelease(g_goalModel);
        g_goalModel = nullptr;
    }

    int   g_clearTex = -1;
    bool  g_prevB = false;
    float g_clearTimer = 0.0f;

    bool IsTrigger(bool current, bool* prev)
    {
        const bool triggered = current && !(*prev);
        *prev = current;
        return triggered;
    }

    bool LoadClearTexture()
    {
        if (g_clearTex >= 0) return true;
        g_clearTex = Texture_Load(L"stage_clear.png");
        return (g_clearTex >= 0);
    }

    void DrawClearCentered()
    {
        if (g_clearTex < 0) return;

        const float screenW = (float)Direct3D_GetBackBufferWidth();
        const float screenH = (float)Direct3D_GetBackBufferHeight();
        const float w = (float)Texture_Width(g_clearTex);
        const float h = (float)Texture_Height(g_clearTex);

        Sprite_Draw(g_clearTex, (screenW - w) * 0.5f-33, (screenH - h) * 0.5f+40,w+65,h);
    }

    AABB MakeGoalAABB()
    {
        if (g_goalModel)
            return Model_GetAABB(g_goalModel, g_goalPos);

        const float r = 1.0f;
        return { {g_goalPos.x - r, g_goalPos.y - r, g_goalPos.z - r},
                 {g_goalPos.x + r, g_goalPos.y + r, g_goalPos.z + r} };
    }
}

void Goal_Init()
{
    g_goalState = GoalState::Active;
    g_goalPos = { 0,0,0 };
    g_prevB = false;
    g_clearTimer = 0.0f;
    g_goalYaw = 0.0f;

    LoadGoalModel();
    LoadClearTexture();
    LoadAudio("clear.wav");
}

void Goal_Uninit()
{
    g_goalState = GoalState::Inactive;
    ReleaseGoalModel();
    UnloadAudio(goalBgm);
}

void Goal_SetPosition(const XMFLOAT3& pos) { g_goalPos = pos; }
XMFLOAT3 Goal_GetPosition() { return g_goalPos; }
GoalState Goal_GetState() { return g_goalState; }

void Goal_Update(double elapsedTime)
{
    if (g_goalState == GoalState::Inactive) return;

    g_goalYaw += (float)elapsedTime;

    if (g_goalState == GoalState::WaitInput)
    {
        GamepadState pad{};
        const bool padConnected = Gamepad_GetState(0, &pad);
        const bool bTrigger = padConnected && IsTrigger(pad.b, &g_prevB);
        const bool enterTrigger = KeyLogger_IsTrigger(KK_ENTER);

        if (bTrigger || enterTrigger)
        {
            Game_ChangeStage(StageId::Title);
            g_goalState = GoalState::Inactive;
        }
        return;
    }

    if (g_goalState == GoalState::Active)
    {
        LoadGoalModel();
        const AABB playerAabb = Player_GetAABB();
        const AABB goalAabb = MakeGoalAABB();

        if (Collision_IsOverlapAABB(playerAabb, goalAabb))
        {
            g_goalState = GoalState::Clear;
            g_clearTimer = 0.0f;
            g_prevB = false;
        }
    }

    if (g_goalState == GoalState::Clear)
    {
        g_clearTimer += (float)elapsedTime;
        if (g_clearTimer >= 0.2f)
            g_goalState = GoalState::WaitInput;
    }
}

void Goal_Draw3D()
{
    if (g_goalState == GoalState::Inactive) return;
    if (!LoadGoalModel()) return;

    const XMMATRIX mtxWorld =
        XMMatrixRotationY(g_goalYaw) *
        XMMatrixTranslation(g_goalPos.x, g_goalPos.y, g_goalPos.z);

    ModelDraw(g_goalModel, mtxWorld);
}

void Goal_DrawUI()
{
    if (!(g_goalState == GoalState::Clear || g_goalState == GoalState::WaitInput))
        return;

    if (!LoadClearTexture()) return;

    Direct3D_SetDepthEnable(false);
    Sprite_Begin();
    DrawClearCentered();
    Direct3D_SetDepthEnable(true);
}

void Goal_Draw()
{
    Goal_DrawUI();
}
