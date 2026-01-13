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
#include<DirectXMath.h>
#include <cmath>

using namespace DirectX;

static XMFLOAT3 no{ 0.0f,0.0f,0.0f };
static float aTime = 0.0f;

static float g_startTimeXZ1 = 0.0f;
static bool  g_isOffsetXZ1 = false;
static float g_prevOffsetX1 = 0.0f;
static float g_prevOffsetZ1 = 0.0f;


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
    g_prevOffsetX1 = 0.0f;
    g_prevOffsetZ1 = 0.0f;
}
bool StageDisapear_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath)
{
    StageDisapear_ResetRuntime();
    const char* loadPath = (jsonPath && jsonPath[0]) ? jsonPath : Stage01_GetCurrentJsonPath();
    const bool loaded = Stage01_LoadJson(loadPath);
    Player_DebugTeleport(position, true);
    return loaded;
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
    const XMFLOAT3& playerPos = Player_GetPosition();

    float deltaX1 = 0.0f; float offsetX1 = g_prevOffsetX1;
    float deltaZ1 = 0.0f; float offsetZ1 = g_prevOffsetZ1;

    {//1
        const StageBlock* cube1 = Stage01_Get(1);
        if (cube1)
        {
            const AABB playerAabb = Player_GetAABB();
            const XMFLOAT3& playerVel = Player_GetVelocity();
            const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

            if (StageDisapear_CanRideBlock(playerAabb, canRidePlatform, *cube1, 0.0f))
            {
                if (!g_isOffsetXZ1)
                {
                    g_startTimeXZ1 = aTime;
                    g_prevOffsetX1 = 0.0f;
                    g_prevOffsetZ1 = 0.0f;
                    g_isOffsetXZ1 = true;
                }
            }
        }
        if (g_isOffsetXZ1) {
            const float t = aTime - g_startTimeXZ1;
            offsetX1 = -3.1f * t;
            offsetZ1 = -0.1f * t;
            deltaX1 = offsetX1 - g_prevOffsetX1;
            deltaZ1 = offsetZ1 - g_prevOffsetZ1;
        }
        g_prevOffsetX1 = offsetX1;
        g_prevOffsetZ1 = offsetZ1;




        Stage01_AddObjectTransform(1, no, { deltaX1,0.0f, deltaZ1 }, no);
    }



}