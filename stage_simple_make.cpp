/*==============================================================================

　　  ステージ01オブジェクト配置[stage_simple_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#include "stage_simple_make.h"
#include "stage01_manage.h"
#include"player.h"
#include<DirectXMath.h>
#include <cmath>

using namespace DirectX;

static XMFLOAT3 no{ 0.0f,0.0f,0.0f };
static float aTime = 0.0f;

static float PrevOffsetY1 = 0.0f;

//上下するCube上のプレイヤーの挙動を正常化する関数
struct StageSimpleRidePlatform
{
    int blockIndex;
    float deltaX;
    float deltaY;
    float deltaZ;
};
static bool StageSimple_ApplyRidePlatforms(const StageSimpleRidePlatform* platforms, int platformCount)
{
    const AABB playerAabb = Player_GetAABB();
    const XMFLOAT3& playerVel = Player_GetVelocity();
    const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);
    constexpr float kGroundEps = 0.06f;

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

        const AABB& box = block->aabb;
        const bool overlapXZ = !(playerAabb.max.x <= box.min.x || playerAabb.min.x >= box.max.x ||
            playerAabb.max.z <= box.min.z || playerAabb.min.z >= box.max.z);
        const float dy = playerAabb.min.y - box.max.y;

        if (overlapXZ && dy >= -0.002f && dy <= kGroundEps && canRidePlatform)
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

void StageSimple_Initialize()
{
}

void StageSimple_Finalize()
{
}

void StageSimple_Update(double elapsedTime)
{
    aTime += static_cast<float>(elapsedTime);

    const float offsetY1 = 5.0f * sinf(aTime);
    const float deltaY1 = offsetY1 - PrevOffsetY1;

    static float offsetX1 = 0.0f;
    offsetX1 += 0.005f * static_cast<float>(elapsedTime);
    if (offsetX1 > 100.0f) offsetX1 = 0.0f;
    const float deltaX1 = offsetX1;

    //※上下左右するCubeには登録必須
    const StageSimpleRidePlatform ridePlatforms[] = {
    { 72, 0.0f, deltaY1, 0.0f },
    { 74, deltaX1, 0.0f, 0.0f } };
    StageSimple_ApplyRidePlatforms(ridePlatforms, sizeof(ridePlatforms) / sizeof(ridePlatforms[0]));

    PrevOffsetY1 = offsetY1;
    Stage01_AddObjectTransform(72, { 0.0f, deltaY1, 0.0f }, no, no);

    Stage01_AddObjectTransform(74, { deltaX1, 0.0f, 0.0f }, no, no);




}