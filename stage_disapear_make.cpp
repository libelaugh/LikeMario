/*==============================================================================

　　  ステージdisapearステージ作り[stage_simple_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/
#include "stage_disapear_make.h"
#include "stage_disapear_manager.h"
#include "stage01_manage.h"
#include"player.h"
#include"collision.h"
#include"debug_text.h"
#include<DirectXMath.h>
#include <cmath>

using namespace DirectX;

hal::DebugText* g_titleText = nullptr;
static XMFLOAT3 no{ 0.0f,0.0f,0.0f };
static float aTime = 0.0f;

static constexpr float kShrinkDuration = 1.0f;   // 1秒で縮む
static constexpr float kMinSize = 0.02f;         // 0にしない（負スケール防止）

static bool  g_shrinking1 = false;
static float g_startTime1 = 0.0f;
static XMFLOAT3 g_prevTargetSizeOffset1{ 0,0,0 };
static bool  g_hidden1 = false;



static void StageDisapear_ResetRuntime();

//上下するCube上のプレイヤーの挙動を正常化する関数
struct StageDisapearRidePlatform
{
    int blockIndex;
    float deltaX;
    float deltaY;
    float deltaZ;
};
static bool StageDisapear_CanRideBlock(const AABB& playerAabb, bool canRidePlatform, const StageBlock& block, float rideUpEps)
{
    constexpr float kGroundEps = 0.06f;
    const AABB& box = block.aabb;
    const bool overlapXZ = !(playerAabb.max.x <= box.min.x || playerAabb.min.x >= box.max.x ||
        playerAabb.max.z <= box.min.z || playerAabb.min.z >= box.max.z);
    const float dy = playerAabb.min.y - box.max.y;

    return overlapXZ && dy >= -(0.002f + rideUpEps) && dy <= kGroundEps && canRidePlatform;
}
static bool StageDisapear_ApplyRidePlatforms(const StageDisapearRidePlatform* platforms, int platformCount)
{
    const AABB playerAabb = Player_GetAABB();
    const XMFLOAT3& playerVel = Player_GetVelocity();
    const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

    for (int i = 0; i < platformCount; ++i)
    {
        const StageDisapearRidePlatform& platform = platforms[i];
        if (std::fabs(platform.deltaX) <= 0.0f && std::fabs(platform.deltaY) <= 0.0f && std::fabs(platform.deltaZ) <= 0.0f)
        {
            continue;
        }

        const StageBlock* block = Stage01_Get(platform.blockIndex);
        if (!block)
        {
            continue;
        }
        const float rideUpEps = (platform.deltaY > 0.0f) ? platform.deltaY : 0.0f;
        if (StageDisapear_CanRideBlock(playerAabb, canRidePlatform, *block, rideUpEps))
        {
            DirectX::XMFLOAT3 pos = Player_GetPosition();
            pos.x += platform.deltaX;
            pos.y += platform.deltaY;
            pos.z += platform.deltaZ;
            Player_DebugTeleport(pos, false);
            return true;
        }
    }

    return false;
}
static void HideStageBlockRuntime(int index)
{
    StageBlock* block = Stage01_GetMutable(index);
    if (!block) return;

    block->positionOffset.y -= 10000.0f;
    block->sizeOffset.x = -block->size.x;
    block->sizeOffset.y = -block->size.y;
    block->sizeOffset.z = -block->size.z;
    Stage01_RebuildObject(index);
}

static void StageDisapear_ResetRuntime()
{
    aTime = 0.0f;
    g_shrinking1 = false;
    g_startTime1 = 0.0f;
    g_prevTargetSizeOffset1 = { 0,0,0 };
    g_hidden1 = false;
}
bool StageDisapear_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath)
{
    StageDisapear_ResetRuntime();
    const char* loadPath = (jsonPath && jsonPath[0]) ? jsonPath : Stage01_GetCurrentJsonPath();
    const bool loaded = Stage01_LoadJson(loadPath);
    Player_DebugTeleport(position, true);
    return loaded;
}
static int FindRiddenBlockIndex()
{
    const AABB playerAabb = Player_GetAABB();
    const XMFLOAT3& playerVel = Player_GetVelocity();
    const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

    const int n = Stage01_GetCount();
    for (int i = 0; i < n; ++i)
    {
        const StageBlock* b = Stage01_Get(i);
        if (!b) continue;
        if (StageDisapear_CanRideBlock(playerAabb, canRidePlatform, *b, 0.0f))
            return i;
    }
    return -1;
}

void StageDisapear_Initialize()
{
}
void StageDisapear_Finalize()
{
}

void StageDisapear_Update(double elapsedTime)
{
    aTime += static_cast<float>(elapsedTime);

    const StageBlock* cube1 = Stage01_Get(1);

    static int g_targetIndex = -1;

    const int rideIndex = FindRiddenBlockIndex();
    if (rideIndex >= 0 && !g_hidden1)
    {
        if (!g_shrinking1)
        {
            g_shrinking1 = true;
            g_targetIndex = rideIndex;
            g_startTime1 = aTime;
            g_prevTargetSizeOffset1 = { 0,0,0 };
        }
    }

    if (g_shrinking1 && g_targetIndex >= 0)
    {
        const StageBlock* cube = Stage01_Get(g_targetIndex);
        if (cube)
        {
            const float t = aTime - g_startTime1;
            float u = (kShrinkDuration > 0.0f) ? (t / kShrinkDuration) : 1.0f;
            if (u < 0.0f) u = 0.0f;
            if (u > 1.0f) u = 1.0f;

            XMFLOAT3 targetOffset{
                (kMinSize - cube->size.x) * u,
                (kMinSize - cube->size.y) * u,
                (kMinSize - cube->size.z) * u
            };

            XMFLOAT3 delta{
                targetOffset.x - g_prevTargetSizeOffset1.x,
                targetOffset.y - g_prevTargetSizeOffset1.y,
                targetOffset.z - g_prevTargetSizeOffset1.z
            };

            Stage01_AddObjectTransform(g_targetIndex, no, delta, no);
            g_titleText->SetText("Transform called!");
            g_titleText->Draw();
            g_prevTargetSizeOffset1 = targetOffset;

            if (u >= 1.0f)
            {
                HideStageBlockRuntime(g_targetIndex);
                g_hidden1 = true;
                g_shrinking1 = false;
                g_targetIndex = -1;
            }
        }
    }



}