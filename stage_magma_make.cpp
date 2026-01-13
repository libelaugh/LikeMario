/*==============================================================================

　　  ステージmagmaステージ作り[stage_magma_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/13
--------------------------------------------------------------------------------

==============================================================================*/

#include "stage_magma_make.h"
#include "stage_magma_manager.h"
#include "stage01_manage.h"
#include"player.h"
#include"collision.h"
#include<DirectXMath.h>
#include <cmath>

using namespace DirectX;

static XMFLOAT3 no{ 0.0f,0.0f,0.0f };
static float aTime = 0.0f;

static void StageMagma_ResetRuntime();

//上下するCube上のプレイヤーの挙動を正常化する関数
struct StageMagmaRidePlatform
{
    int blockIndex;
    float deltaX;
    float deltaY;
    float deltaZ;
};
static bool StageMagma_CanRideBlock(const AABB& playerAabb, bool canRidePlatform, const StageBlock& block, float rideUpEps)
{
    constexpr float kGroundEps = 0.06f;
    const AABB& box = block.aabb;
    const bool overlapXZ = !(playerAabb.max.x <= box.min.x || playerAabb.min.x >= box.max.x ||
        playerAabb.max.z <= box.min.z || playerAabb.min.z >= box.max.z);
    const float dy = playerAabb.min.y - box.max.y;

    return overlapXZ && dy >= -(0.002f + rideUpEps) && dy <= kGroundEps && canRidePlatform;
}
static bool StageSimple_ApplyRidePlatforms(const StageMagmaRidePlatform* platforms, int platformCount)
{
    const AABB playerAabb = Player_GetAABB();
    const XMFLOAT3& playerVel = Player_GetVelocity();
    const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

    for (int i = 0; i < platformCount; ++i)
    {
        const StageMagmaRidePlatform& platform = platforms[i];
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
        if (StageMagma_CanRideBlock(playerAabb, canRidePlatform, *block, rideUpEps))
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

static void StageMagma_ResetRuntime()
{
    aTime = 0.0f;

}
bool StageMagma_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath)
{
    StageMagma_ResetRuntime();
    const char* loadPath = (jsonPath && jsonPath[0]) ? jsonPath : Stage01_GetCurrentJsonPath();
    const bool loaded = Stage01_LoadJson(loadPath);
    Player_DebugTeleport(position, true);
    return loaded;
}


void StageMagma_Initialize()
{
}
void StageMagma_Finalize()
{
}

void StageMagma_Update(double elapsedTime)
{

}
