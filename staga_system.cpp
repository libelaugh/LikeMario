#include "staga_system.h"
#include "stage_simple_manager.h"
#include "stage_magma_manager.h"

static StageId g_cur = StageId::Stage1;
static bool    g_hasReq = false;
static StageId g_req = StageId::Stage1;

static bool StageSystem_IsValidStage(StageId id)
{
    const int idx = static_cast<int>(id);
    return idx >= 0 && idx < static_cast<int>(StageId::Count);
}

void StageSystem_Initialize(StageId first)
{
    if (!StageSystem_IsValidStage(first))
        first = StageId::Stage1;
    g_cur = first;
    g_req = first;
    g_hasReq = false;
    const auto& s = GetStageInfo(g_cur);
    StageSimpleManager_Initialize(s);
}

void StageSystem_Finalize()
{
    StageSimpleManager_Finalize();
}

void StageSystem_RequestChange(StageId next)
{
    if (!StageSystem_IsValidStage(next))
        return;
    g_req = next;
    g_hasReq = true;
}

void StageSystem_RequestNext()
{
    const int count = static_cast<int>(StageId::Count);
    const int next = (static_cast<int>(g_cur) + 1) % count;
    StageSystem_RequestChange(static_cast<StageId>(next));
}

void StageSystem_RequestPrev()
{
    const int count = static_cast<int>(StageId::Count);
    const int prev = (static_cast<int>(g_cur) + count - 1) % count;
    StageSystem_RequestChange(static_cast<StageId>(prev));
}

void StageSystem_Update(double dt)
{
    if (g_hasReq)
    {
        g_hasReq = false;


        g_cur = g_req;
        const auto& s = GetStageInfo(g_cur);
        StageSimpleManager_ChangeStage(s);
    }

    StageSimpleManager_Update(dt);
}

void StageSystem_Draw()
{
    StageSimpleManager_Draw();
}

StageId StageSystem_GetCurrent() { return g_cur; }
