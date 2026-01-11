/*==============================================================================

　　  ステージ01オブジェクト配置[stage_simple_make.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#include "stage_simple_make.h"
#include"stage_map.h"
#include<DirectXMath.h>

using namespace DirectX;

static XMFLOAT3 no{0.0f,0.0f,0.0f};
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
    const float offsetY = 54.0f * sinf(aTime);
    const float deltaY = offsetY - aPrevOffsetY;
    aPrevOffsetY = offsetY;
    Map_AddObjectTransform(72, { 0.0f, deltaY, 0.0f }, no, no);
}
