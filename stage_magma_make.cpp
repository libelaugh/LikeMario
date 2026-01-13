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

static float g_startTimeXY1 = 0.0f;   
static bool  g_isOffsetXY1 = false;   
static float g_prevOffsetX1 = 0.0f;   
static float g_prevOffsetY1 = 0.0f;   
static float g_startTimeXY2 = 0.0f;   
static bool  g_isOffsetXY2 = false;   
static float g_prevOffsetX2 = 0.0f;   
static float g_prevOffsetY2 = 0.0f;   
static float g_startTimeXY3 = 0.0f;   
static bool  g_isOffsetXY3 = false;   
static float g_prevOffsetX3 = 0.0f;   
static float g_prevOffsetY3 = 0.0f;   
static float g_startTimeXY4 = 0.0f;   
static bool  g_isOffsetXY4 = false;   
static float g_prevOffsetX4 = 0.0f;   
static float g_prevOffsetY4 = 0.0f;   
   
static float g_prevOffsetY5 = 0.0f;   
static float g_prevOffsetY6 = 0.0f;   
static float g_prevOffsetY7 = 0.0f;   
static float g_prevOffsetY8 = 0.0f;   
static float g_prevOffsetY9 = 0.0f;   
static float g_prevOffsetY10 = 0.0f;

static float g_prevOffsetY11 = 0.0f;

static float g_prevMeshOffsetY = 0.0f;
static float meshOffsetY = 30.0f;

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
static bool StageMagma_ApplyRidePlatforms(const StageMagmaRidePlatform* platforms, int platformCount)
{
    const AABB playerAabb = Player_GetAABB();
    const XMFLOAT3& playerVel = Player_GetVelocity();
    const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

    for (int i = 0; i < platformCount; ++i)
    {
        const StageMagmaRidePlatform& platform = platforms[i];
        constexpr float kDeltaEps = 1.0e-6f;
        if (std::fabs(platform.deltaX) <= kDeltaEps && std::fabs(platform.deltaY) <= kDeltaEps && std::fabs(platform.deltaZ) <= kDeltaEps)
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
    g_startTimeXY1 = 0.0f;
    g_isOffsetXY1 = false;
    g_prevOffsetX1 = 0.0f;
    g_prevOffsetY1 = 0.0f;
    g_startTimeXY2 = 0.0f;
    g_isOffsetXY2 = false;
    g_prevOffsetX2 = 0.0f;
    g_prevOffsetY2 = 0.0f;
    g_startTimeXY3 = 0.0f;
    g_isOffsetXY3 = false;
    g_prevOffsetX3 = 0.0f;
    g_prevOffsetY3 = 0.0f;
    g_startTimeXY4 = 0.0f;
    g_isOffsetXY4 = false;
    g_prevOffsetX4 = 0.0f;
    g_prevOffsetY4 = 0.0f;

    g_prevOffsetY5 = 0.0f;
    g_prevOffsetY6 = 0.0f;
    g_prevOffsetY7 = 0.0f;
    g_prevOffsetY8 = 0.0f;
    g_prevOffsetY9 = 0.0f;
    g_prevOffsetY10 = 0.0f;

    g_prevOffsetY11 = 0.0f;

    g_prevMeshOffsetY = 0.0f;
    meshOffsetY = 30.0f;
}
bool StageMagma_SetPlayerPositionAndLoadJson(const DirectX::XMFLOAT3& position, const char* jsonPath)
{
    StageMagma_ResetRuntime();
    StageMagmaManager_SetMagmaY(StageMagmaManager_GetMagmaBaseY());
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
    aTime += (float)elapsedTime;
    const XMFLOAT3& playerPos = Player_GetPosition();
    float deltaY = 0.0f;
    float offsetY = g_prevMeshOffsetY;
    offsetY = 0.15f * aTime;
    deltaY = offsetY - g_prevMeshOffsetY;
    StageMagmaManager_AddMagmaY(deltaY);
    g_prevMeshOffsetY = offsetY;
    
    float deltaX1 = 0.0f; float offsetX1 = g_prevOffsetX1;
    float deltaY1 = 0.0f; float offsetY1 = g_prevOffsetY1;
    float deltaX2 = 0.0f; float offsetX2 = g_prevOffsetX2;
    float deltaY2 = 0.0f; float offsetY2 = g_prevOffsetY2;
    float deltaX3 = 0.0f; float offsetX3 = g_prevOffsetX3;
    float deltaY3 = 0.0f; float offsetY3 = g_prevOffsetY3;
    float deltaX4 = 0.0f; float offsetX4 = g_prevOffsetX4;
    float deltaY4 = 0.0f; float offsetY4 = g_prevOffsetY4;
    const float offsetY5 = 3.0f * sinf(aTime);
    const float deltaY5 = offsetY5 - g_prevOffsetY5;
    const float offsetY6 = -3.0f * sinf(aTime);
    const float deltaY6 = offsetY6 - g_prevOffsetY6;
    const float offsetY7 = 3.0f * sinf(aTime);
    const float deltaY7 = offsetY7- g_prevOffsetY7;
    const float offsetY8 = -3.0f * sinf(aTime);
    const float deltaY8 = offsetY8 - g_prevOffsetY8;
    const float offsetY9 = -3.0f * sinf(aTime);
    const float deltaY9 = offsetY9 - g_prevOffsetY9;
    const float offsetY10 = 3.0f * sinf(aTime);
    const float deltaY10 = offsetY10 - g_prevOffsetY10;

    const float offsetY11 = 7.0f * sinf(aTime);
    const float deltaY11 = offsetY11 - g_prevOffsetY11;

    g_prevOffsetY5 = offsetY5; g_prevOffsetY6 = offsetY6; g_prevOffsetY7 = offsetY7;
    g_prevOffsetY8 = offsetY8; g_prevOffsetY9 = offsetY9; g_prevOffsetY10 = offsetY10;

    g_prevOffsetY11 = offsetY11;
    

    {
        {//1
            const StageBlock* cube1 = Stage01_Get(1);
            if (cube1)
            {
                const AABB playerAabb = Player_GetAABB();
                const XMFLOAT3& playerVel = Player_GetVelocity();
                const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

                if (StageMagma_CanRideBlock(playerAabb, canRidePlatform, *cube1, 0.0f))
                {
                    if (!g_isOffsetXY1)
                    {
                        g_startTimeXY1 = aTime;
                        g_prevOffsetX1 = 0.0f;
                        g_isOffsetXY1 = true;
                        g_prevOffsetY1 = 0.0f;
                    }
                }
            }
            if (g_isOffsetXY1) {
                const float t = aTime - g_startTimeXY1;
                offsetX1 = 0.4f * t;
                offsetY1 = -0.4f * t;
                deltaX1 = offsetX1 - g_prevOffsetX1;
                deltaY1 = offsetY1 - g_prevOffsetY1;
            }
            g_prevOffsetX1 = offsetX1;
            g_prevOffsetY1 = offsetY1;
        }
        {//2
            const StageBlock* cube2 = Stage01_Get(2);
            if (cube2)
            {
                const AABB playerAabb = Player_GetAABB();
                const XMFLOAT3& playerVel = Player_GetVelocity();
                const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

                if (StageMagma_CanRideBlock(playerAabb, canRidePlatform, *cube2, 0.0f))
                {
                    if (!g_isOffsetXY2)
                    {
                        g_startTimeXY2 = aTime;
                        g_prevOffsetX2 = 0.0f;
                        g_isOffsetXY2 = true;
                        g_prevOffsetY2 = 0.0f;
                    }
                }
            }

            if (g_isOffsetXY2) {
                const float t = aTime - g_startTimeXY2;
                offsetX2 = -0.4f * t;
                offsetY2 = -0.4f * t;
                deltaX2 = offsetX2 - g_prevOffsetX2;
                deltaY2 = offsetY2 - g_prevOffsetY2;
            }

            g_prevOffsetX2 = offsetX2;
            g_prevOffsetY2 = offsetY2;
        }
        {//3
            const StageBlock* cube3 = Stage01_Get(3);
            if (cube3)
            {
                const AABB playerAabb = Player_GetAABB();
                const XMFLOAT3& playerVel = Player_GetVelocity();
                const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

                if (StageMagma_CanRideBlock(playerAabb, canRidePlatform, *cube3, 0.0f))
                {
                    if (!g_isOffsetXY3)
                    {
                        g_startTimeXY3 = aTime;
                        g_prevOffsetX3 = 0.0f;
                        g_isOffsetXY3 = true;
                        g_prevOffsetY3 = 0.0f;
                    }
                }
            }

            if (g_isOffsetXY3) {
                const float t = aTime - g_startTimeXY3;
                offsetX3 = 0.4f * t;
                offsetY3 = -0.4f * t;
                deltaX3 = offsetX3 - g_prevOffsetX3;
                deltaY3 = offsetY3 - g_prevOffsetY3;
            }

            g_prevOffsetX3 = offsetX3;
            g_prevOffsetY3 = offsetY3;
        }
        {//4
            const StageBlock* cube4 = Stage01_Get(4);
            if (cube4)
            {
                const AABB playerAabb = Player_GetAABB();
                const XMFLOAT3& playerVel = Player_GetVelocity();
                const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

                if (StageMagma_CanRideBlock(playerAabb, canRidePlatform, *cube4, 0.0f))
                {
                    if (!g_isOffsetXY4)
                    {
                        g_startTimeXY4 = aTime;
                        g_prevOffsetX4 = 0.0f;
                        g_isOffsetXY4 = true;
                        g_prevOffsetY4 = 0.0f;
                    }
                }
            }

            if (g_isOffsetXY4) {
                const float t = aTime - g_startTimeXY4;
                offsetX4 = -0.4f * t;
                offsetY4 = -0.4f * t;
                deltaX4 = offsetX4 - g_prevOffsetX4;
                deltaY4 = offsetY4 - g_prevOffsetY4;
            }

            g_prevOffsetX4 = offsetX4;
            g_prevOffsetY4 = offsetY4;
        }
    }

        const StageMagmaRidePlatform ridePlatforms[] = {
        { 1, deltaX1, deltaY1, 0.0f },{ 2, deltaX2, deltaY2, 0.0f },{ 3, deltaX3, deltaY3, 0.0f },{ 4, deltaX4, deltaY4, 0.0f },
        { 14, 0.0f, deltaY5, 0.0f },{ 15, 0.0f, deltaY6, 0.0f },{ 16, 0.0f, deltaY7, 0.0f },
        { 17, 0.0f, deltaY8, 0.0f },{ 18, 0.0f, deltaY9, 0.0f },{ 19, 0.0f, deltaY10, 0.0f },
        { 42, 0.0f, deltaY11, 0.0f },  };
        StageMagma_ApplyRidePlatforms(ridePlatforms, sizeof(ridePlatforms) / sizeof(ridePlatforms[0]));

        Stage01_AddObjectTransform(1, { deltaX1, deltaY1, 0.0f }, no, no);
        Stage01_AddObjectTransform(2, { deltaX2, deltaY2, 0.0f }, no, no);
        Stage01_AddObjectTransform(3, { deltaX3, deltaY3, 0.0f }, no, no);
        Stage01_AddObjectTransform(4, { deltaX4, deltaY4, 0.0f }, no, no);

        Stage01_AddObjectTransform(14, {0.0f, deltaY5, 0.0f }, no, no); Stage01_AddObjectTransform(15, { 0.0f, deltaY6, 0.0f }, no, no);
        Stage01_AddObjectTransform(16, { 0.0f, deltaY7, 0.0f }, no, no); Stage01_AddObjectTransform(17, { 0.0f, deltaY8, 0.0f }, no, no);
        Stage01_AddObjectTransform(18, { 0.0f, deltaY9, 0.0f }, no, no); Stage01_AddObjectTransform(19, { 0.0f, deltaY10, 0.0f }, no, no);

        Stage01_AddObjectTransform(42, { 0.0f, deltaY11, 0.0f }, no, no);



        if (playerPos.y <StageMagmaManager_GetMagmaY()-0.5f) {
            StageMagma_ResetRuntime();
            StageMagmaManager_SetMagmaY(StageMagmaManager_GetMagmaBaseY());
            const XMFLOAT3 spawnPos = StageMagmaManager_GetSpawnPosition();
            StageMagma_SetPlayerPositionAndLoadJson(spawnPos, nullptr);
        }
}
