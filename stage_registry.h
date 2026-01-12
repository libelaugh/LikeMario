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
    Stage1 = 0,
    Stage2,
    Stage3,
    Stage4,
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
        { "Stage 1", "stage_simple.json", {0,5,2.5f}, {0,0,1} },
        { "Stage 2", "stage02.json",      {0,5,2.5f}, {0,0,1} },
        { "Stage 3", "stage03.json",      {0,5,2.5f}, {0,0,1} },
        { "Stage 4", "stage04.json",      {0,5,2.5f}, {0,0,1} },
    };
    return kStages[(int)id];
}
#endif//STAGE_REGISTRY_H