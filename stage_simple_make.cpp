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
static XMFLOAT3 aPos1{};

void StageSimple_Initialize()
{
}

void StageSimple_Finalize()
{
}

void StageSimple_Update(double elapsedTime)
{
    aPos1.y += 5.0f*sinf(elapsedTime);
    Map_AddObjectTransform(72, aPos1, no, no);
}
