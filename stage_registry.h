/*==============================================================================

　　  ステージ登録[stage_registry.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_REGISTRY_H
#define STAGE_REGISTRY_H

#include <DirectXMath.h>

// Note:
// - 0..(Title-1) are playable stages.
// - Title / Clear are UI states and must NOT be passed to StageSystem / GetStageInfo.

enum class StageId : int
{
    StageSimple = 0,
    StageMagma,
    StageDisapear,
    StageInvisible,

    Title,
    Clear,

    Count
};

struct StageInfo
{
    const char* displayName;   // title screen display name
    const char* jsonPath;      // Stage01_LoadJson / Initialize path
    DirectX::XMFLOAT3 spawnPos;
    DirectX::XMFLOAT3 spawnFront;
};

// Playable stage count (Simple..Invisible)
constexpr int kPlayableStageCount = static_cast<int>(StageId::Title);

inline bool StageId_IsPlayable(StageId id)
{
    const int idx = static_cast<int>(id);
    return idx >= 0 && idx < kPlayableStageCount;
}

inline const StageInfo& GetStageInfo(StageId id)
{
    static const StageInfo kStages[kPlayableStageCount] =
    {
        { "Stage Simple",    "stage_simple.json",    { 0, 5,  2.5f }, { 0, 0, 1 } },
        { "Stage Magma",     "stage_magma.json",     { 0, 5, -9.0f }, { 0, 0, 1 } },
        { "Stage Disapear",  "stage_disapear.json",  { 0, 5,  2.5f }, { 0, 0, 1 } },
        { "Stage Invisible", "stage_invisible.json", { 0, 5,  2.5f }, { 0, 0, 1 } },
    };

    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= kPlayableStageCount)
        idx = 0; // safety fallback

    return kStages[idx];
}
#endif//STAGE_REGISTRY_H