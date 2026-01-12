#include "staga_system.h"
#include "stage_simple_manager.h"
#include "stage_magma_manager.h"

static StageId g_cur = StageId::Stage1;
static bool    g_hasReq = false;
static StageId g_req = StageId::Stage1;

void StageSystem_Initialize(StageId first)
{
    g_cur = first;
    const auto& s = GetStageInfo(g_cur);
    //StageSimpleManager_Initialize(s.jsonPath, s.spawnPos, s.spawnFront);
}

void StageSystem_Finalize()
{
    StageSimpleManager_Finalize();
}

void StageSystem_RequestChange(StageId next)
{
    g_req = next;
    g_hasReq = true;
}

void StageSystem_Update(double dt)
{
    if (g_hasReq)
    {
        g_hasReq = false;

        // フェードがあるならここで「フェードアウト完了後」にやるのが理想
        StageSimpleManager_Finalize();

        g_cur = g_req;
        const auto& s = GetStageInfo(g_cur);
        //StageSimpleManager_Initialize(s.jsonPath, s.spawnPos, s.spawnFront);
    }

    StageSimpleManager_Update(dt);
}

void StageSystem_Draw()
{
    StageSimpleManager_Draw();
}

StageId StageSystem_GetCurrent() { return g_cur; }
