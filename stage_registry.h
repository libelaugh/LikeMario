/*==============================================================================

　　  ステージ登録[stage_registry.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_REGISTRY_H
#define STAGE_REGISTRY_H

#include <DirectXMath.h>

enum class StageId : int
{
    StageSimple = 0,
    StageMagma,
    StageDisapear,
    StageInvisible,
    Count
};

struct StageInfo
{
    const char* displayName;   // タイトル画面表示名
    const char* jsonPath;      // Stage01_LoadJson / Initialize に渡す
    DirectX::XMFLOAT3 spawnPos;
    DirectX::XMFLOAT3 spawnFront;
};

inline const StageInfo& GetStageInfo(StageId id)
{
    static const StageInfo kStages[] =
    {
        { "Stage Simple", "stage_simple.json", {0,5,2.5f}, {0,0,1} },
        { "Stage Magma", "stage_magma.json",      {0,5,2.5f}, {0,0,1} },
        { "Stage Disapear", "stage_disapear.json",      {0,5,2.5f}, {0,0,1} },
        { "Stage Invisible", "stage_invisible.json",      {0,5,2.5f}, {0,0,1} },
    };
    return kStages[(int)id];
}
#endif//STAGE_REGISTRY_H