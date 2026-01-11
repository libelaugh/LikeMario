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

static float aPrevOffsetY = 0.0f;

void StageSimple_Initialize()
{
}

void StageSimple_Finalize()
{
}

void StageSimple_Update(double elapsedTime)
{
    aTime += static_cast<float>(elapsedTime);

    const float offsetY = 5.0f * sinf(aTime);
    const float deltaY = offsetY - aPrevOffsetY;

    bool ridePlatform = false;
    if (std::fabs(deltaY) > 0.0f)
    {
        const StageBlock* block = Stage01_Get(72);
        if (block)
        {
            const AABB playerAabb = Player_GetAABB();
            const AABB& box = block->aabb;

            const bool overlapXZ = !(playerAabb.max.x <= box.min.x || playerAabb.min.x >= box.max.x ||
                playerAabb.max.z <= box.min.z || playerAabb.min.z >= box.max.z);
            const float dy = playerAabb.min.y - box.max.y;
            constexpr float kGroundEps = 0.06f;

            const XMFLOAT3& playerVel = Player_GetVelocity();
            const bool canRidePlatform = Player_IsGrounded() && (playerVel.y <= 0.01f);

            ridePlatform = overlapXZ && dy >= -0.002f && dy <= kGroundEps && canRidePlatform;
        }
    }

    aPrevOffsetY = offsetY;
    Stage01_AddObjectTransform(72, { 0.0f, deltaY, 0.0f }, no, no);

    if (ridePlatform)
    {
        DirectX::XMFLOAT3 pos = Player_GetPosition();
        pos.y += deltaY;
        Player_DebugTeleport(pos, false);
    }

}