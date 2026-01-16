/*==============================================================================

　　  ステージsimple作り[stage_simple_make.cpp]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#include "stage_simple_make.h"
#include "stage_simple_manager.h"
#include "stage01_manage.h"
#include"player.h"
#include"collision.h"
#include<DirectXMath.h>
#include <cmath>

using namespace DirectX;

static XMFLOAT3 no{ 0.0f,0.0f,0.0f };
static float aTime = 0.0f;

static float g_prevOffsetY1 = 0.0f;
static float g_prevOffsetX1 = 0.0f;
static float g_prevOffsetX2 = 0.0f;
static float g_prevOffsetX3 = 0.0f;
static float g_prevOffsetX4 = 0.0f;
static float g_prevOffsetX5 = 0.0f;
static float g_prevOffsetX6 = 0.0f;
static float g_startTimeZ7 = 0.0f;
static bool  g_isOffsetZ7 = false;
static float g_prevOffsetZ7 = 0.0f;
static float g_prevOffsetX7 = 0.0f;
static float g_prevOffsetX8 = 0.0f;

static void StageSimple_ResetRuntime();

//上下するCube上のプレイヤーの挙動を正常化する関数
struct StageSimpleRidePlatform
{
    int blockIndex;
    float deltaX;
    float deltaY;
    float deltaZ;
};
static bool StageSimple_CanRideBlock(const AABB& playerAabb, bool canRidePlatform, const StageBlock& block, float rideUpEps)
{
    constexpr float kGroundEps = 0.06f;
    const AABB& box = block.aabb;
    const bool overlapXZ = !(playerAabb.max.x <= box.min.x || playerAabb.min.x >= box.max.x ||
        playerAabb.max.z <= box.min.z || playerAabb.min.z >= box.max.z);
    const float dy = playerAabb.min.y - box.max.y;

    return overlapXZ && dy >= -(0.002f + rideUpEps) && dy <= kGroundEps && canRidePlatform;
}
static bool StageSimple_ApplyRidePlatforms(const StageSimpleRidePlatform* platforms, int platformCount)
{
    const AABB playerAabb = Player_GetAABB();
    const XMFLOAT3& playerVel = Player_GetVelocity();
    const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

    for (int i = 0; i < platformCount; ++i)
    {
        const StageSimpleRidePlatform& platform = platforms[i];
        if (std::fabs(platform.deltaX) <= 0.0f && std::fabs(platform.deltaY) <= 0.0f && std::fabs(platform.deltaZ) <= 0.0f)
            {
            continue;
            }
        
            const StageBlock * block = Stage01_Get(platform.blockIndex);
        if (!block)
        {
            continue;
        }
        const float rideUpEps = (platform.deltaY > 0.0f) ? platform.deltaY : 0.0f;
        if (StageSimple_CanRideBlock(playerAabb, canRidePlatform, *block, rideUpEps))
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

static void StageSimple_ResetRuntime()
{
    aTime = 0.0f;
    g_prevOffsetY1 = 0.0f;
    g_prevOffsetX1 = 0.0f;
    g_prevOffsetX2 = 0.0f;
    g_prevOffsetX3 = 0.0f;
    g_prevOffsetX4 = 0.0f;
    g_prevOffsetX5 = 0.0f;
    g_prevOffsetX6 = 0.0f;
    g_startTimeZ7 = 0.0f;
    g_isOffsetZ7 = false;
    g_prevOffsetZ7 = 0.0f;
    g_prevOffsetX7 = 0.0f;
    g_prevOffsetX8 = 0.0f;
}
bool StageSimple_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath)
{
    StageSimple_ResetRuntime();
    const char* loadPath = (jsonPath && jsonPath[0]) ? jsonPath : Stage01_GetCurrentJsonPath();
    const bool loaded = Stage01_LoadJson(loadPath);
    Player_DebugTeleport(position, true);
    return loaded;
}

void StageSimple_Initialize()
{
}

void StageSimple_Finalize()
{
}

void StageSimple_Update(double elapsedTime)
{
    aTime += static_cast<float>(elapsedTime);
    const XMFLOAT3& playerPos = Player_GetPosition();

    const float offsetY1 = 3.0f * sinf(aTime);
    const float deltaY1 = offsetY1 - g_prevOffsetY1;
    const float offsetX1 = 5.0f*sinf(aTime);
    const float deltaX1 = offsetX1 - g_prevOffsetX1;
    const float offsetX2 = 5.0f * sinf(aTime);
    const float deltaX2 = offsetX2 - g_prevOffsetX2;
    const float offsetX3 = 5.0f * sinf(aTime);
    const float deltaX3 = offsetX3 - g_prevOffsetX3;
    const float offsetX4 = -5.0f * sinf(aTime);
    const float deltaX4 = offsetX4 - g_prevOffsetX4;
    const float offsetX5 = -5.0f * sinf(aTime);
    const float deltaX5 = offsetX5 - g_prevOffsetX5;
    const float offsetX6 = 5.0f * sinf(aTime);
    const float deltaX6 = offsetX6 - g_prevOffsetX6;
    float deltaZ7 = 0.0f;
    float offsetZ7 = g_prevOffsetZ7;
    const float offsetX7 = 6.0f * sinf(aTime*0.7f);
    const float deltaX7 = offsetX7 - g_prevOffsetX7;
    const float offsetX8 = 5.0f * sinf(aTime);
    const float deltaX8 = offsetX8 - g_prevOffsetX8;
    

    g_prevOffsetY1 = offsetY1;
    g_prevOffsetX1 = offsetX1; g_prevOffsetX2 = offsetX2; g_prevOffsetX3 = offsetX3;
    g_prevOffsetX4 = offsetX4; g_prevOffsetX5 = offsetX5; g_prevOffsetX6 = offsetX6;
    g_prevOffsetZ7 = offsetZ7; g_prevOffsetX7 = offsetX7; g_prevOffsetX8 = offsetX8;
    //sin往復
    Stage01_AddObjectTransform(72, { 0.0f, deltaY1, 0.0f }, no, no);
    {
        Stage01_AddObjectTransform(74, { deltaX1, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(75, { deltaX2, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(76, { deltaX3, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(77, { deltaX4, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(78, { deltaX5, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(79, { deltaX6, 0.0f, 0.0f }, no, no);
    }
    Stage01_AddObjectTransform(83, { deltaX7, 0.0f, 0.0f }, no, no);
    Stage01_AddObjectTransform(124, { deltaX8, 0.0f, 0.0f }, no, no);

    {//直進
        const StageBlock* cube81 = Stage01_Get(81);
        if (cube81)
        {
            const AABB playerAabb = Player_GetAABB();
            const XMFLOAT3& playerVel = Player_GetVelocity();
            const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

            if (StageSimple_CanRideBlock(playerAabb, canRidePlatform, *cube81, 0.0f))
            {
                if (!g_isOffsetZ7)
                {
                    g_startTimeZ7 = aTime;
                    g_prevOffsetZ7 = 0.0f;
                    g_isOffsetZ7 = true;
                }
            }
        }
        if (g_isOffsetZ7) {
            const float t = aTime - g_startTimeZ7;
            offsetZ7 = 0.8f * t;
            deltaZ7 = offsetZ7 - g_prevOffsetZ7;
        }
        g_prevOffsetZ7 = offsetZ7;
    }
    

        //※上下左右するCubeには登録必須
        const StageSimpleRidePlatform ridePlatforms[] = {
        { 72, 0.0f, deltaY1, 0.0f },
        { 74, deltaX1, 0.0f, 0.0f },{ 75, deltaX2, 0.0f, 0.0f },{ 76, deltaX3, 0.0f, 0.0f },
        { 77, deltaX4, 0.0f, 0.0f }, { 78, deltaX5, 0.0f, 0.0f }, { 79, deltaX6, 0.0f, 0.0f },
        { 81, 0.0f, 0.0f, deltaZ7 }, { 83, deltaX7, 0.0f, 0.0f },{ 124, deltaX8, 0.0f, 0.0f }, };
        StageSimple_ApplyRidePlatforms(ridePlatforms, sizeof(ridePlatforms) / sizeof(ridePlatforms[0]));


        Stage01_AddObjectTransform(81, { 0.0f, 0.0f, deltaZ7 }, no, no);
        if (playerPos.z > 180.0f) { HideStageBlockRuntime(81); }


        if(playerPos.y<-10.0f){
            StageSimple_ResetRuntime();
            const XMFLOAT3 spawnPos = StageSimpleManager_GetSpawnPosition();
            StageSimple_SetPlayerPositionAndLoadJson(spawnPos, nullptr);
        }
}