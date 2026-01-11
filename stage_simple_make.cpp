/*==============================================================================

　　  ステージ01オブジェクト配置[stage_simple_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#include "stage_simple_make.h"
#include "stage01_manage.h"
#include"player.h"
#include"collision.h"
#include<DirectXMath.h>
#include <cmath>

using namespace DirectX;

static XMFLOAT3 no{ 0.0f,0.0f,0.0f };
static float aTime = 0.0f;



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

    static float prevOffsetY1 = 0.0f;
    const float offsetY1 = 5.0f * sinf(aTime);
    const float deltaY1 = offsetY1 - prevOffsetY1;

    static float prevOffsetX1 = 0.0f;
    const float offsetX1 = 5.0f*sinf(aTime);
    const float deltaX1 = offsetX1 - prevOffsetX1;
    static float prevOffsetX2 = 0.0f;
    const float offsetX2 = 5.0f * sinf(aTime);
    const float deltaX2 = offsetX2 - prevOffsetX2;
    static float prevOffsetX3 = 0.0f;
    const float offsetX3 = 5.0f * sinf(aTime);
    const float deltaX3 = offsetX3 - prevOffsetX3;
    static float prevOffsetX4 = 0.0f;
    const float offsetX4 = -5.0f * sinf(aTime);
    const float deltaX4 = offsetX4 - prevOffsetX4;
    static float prevOffsetX5 = 0.0f;
    const float offsetX5 = -5.0f * sinf(aTime);
    const float deltaX5 = offsetX5 - prevOffsetX5;
    static float prevOffsetX6 = 0.0f;
    const float offsetX6 = 5.0f * sinf(aTime);
    const float deltaX6 = offsetX6 - prevOffsetX6;

    static bool  isOffsetZ7 = false;
    static float startTimeZ7 = 0.0f;
    static float prevOffsetZ7 = 0.0f;
    float deltaZ7 = 0.0f; float offsetZ7{};

    //※上下左右するCubeには登録必須
    const StageSimpleRidePlatform ridePlatforms[] = {
    { 72, 0.0f, deltaY1, 0.0f },
    { 74, deltaX1, 0.0f, 0.0f },{ 75, deltaX2, 0.0f, 0.0f },{ 76, deltaX3, 0.0f, 0.0f },
    { 77, deltaX4, 0.0f, 0.0f }, { 78, deltaX5, 0.0f, 0.0f }, { 79, deltaX6, 0.0f, 0.0f },
    { 81, 0.0f, 0.0f, deltaZ7 }, };
    StageSimple_ApplyRidePlatforms(ridePlatforms, sizeof(ridePlatforms) / sizeof(ridePlatforms[0]));

    prevOffsetY1 = offsetY1;
    prevOffsetX1 = offsetX1;prevOffsetX2 = offsetX2;prevOffsetX3 = offsetX3;
    prevOffsetX4 = offsetX4;prevOffsetX5 = offsetX5;prevOffsetX6 = offsetX6;
    prevOffsetZ7 = offsetZ7;

    Stage01_AddObjectTransform(72, { 0.0f, deltaY1, 0.0f }, no, no);
    {
        Stage01_AddObjectTransform(74, { deltaX1, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(75, { deltaX2, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(76, { deltaX3, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(77, { deltaX4, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(78, { deltaX5, 0.0f, 0.0f }, no, no);
        Stage01_AddObjectTransform(79, { deltaX6, 0.0f, 0.0f }, no, no);
    }

    //XMFLOAT3 pos = Player_GetPosition(); AABB player = Player_ConvertPositionToAABB(XMLoadFloat3(&pos));
    const StageBlock* cube81 = Stage01_Get(81);
    if (cube81 && Collision_IsOverlapAABB(Player_GetAABB(), cube81->aabb)) {
        if (!isOffsetZ7) {                 // 初回起動の瞬間だけ
            isOffsetZ7 = true;
            startTimeZ7 = aTime;          // 起動時刻
            prevOffsetZ7 = 0.0f;           // 相対オフセットにする
        }
    }

    if (isOffsetZ7) {
        const float t = aTime - startTimeZ7;   // 起動後の経過時間
        const float offsetZ7 = 0.5f * t;       // 速度0.5で前進
        deltaZ7 = offsetZ7 - prevOffsetZ7;
        prevOffsetZ7 = offsetZ7;
    }
        Stage01_AddObjectTransform(81, { 0.0f, 0.0f, deltaZ7 }, no, no);
    


}