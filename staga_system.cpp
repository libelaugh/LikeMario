#include "staga_system.h"
#include "stage_simple_manager.h"
#include "stage_magma_manager.h"

// StageSystem routes calls to each stage manager.
// NOTE: Only playable stages (StageId::StageSimple .. StageId::StageInvisible) are valid here.

namespace
{
    enum class StageImpl { Simple, Magma };

    static StageId   g_cur = StageId::StageSimple;
    static bool      g_hasReq = false;
    static StageId   g_req = StageId::StageSimple;

    static bool      g_inited = false;
    static StageImpl g_implCur = StageImpl::Simple;

    static StageImpl ImplFor(StageId id)
    {
        // Magmaだけ専用。その他はSimple系マネージャで処理（Disapear/Invisible等もここに入る）
        return (id == StageId::StageMagma) ? StageImpl::Magma : StageImpl::Simple;
    }

    static void FinalizeCurrent()
    {
        if (!g_inited) return;

        switch (g_implCur)
        {
        case StageImpl::Simple: StageSimpleManager_Finalize(); break;
        case StageImpl::Magma:  StageMagmaManager_Finalize();  break;
        }
        g_inited = false;
    }

    static void InitializeStage(StageId id)
    {
        const StageInfo& info = GetStageInfo(id);
        g_implCur = ImplFor(id);

        switch (g_implCur)
        {
        case StageImpl::Simple: StageSimpleManager_Initialize(info); break;
        case StageImpl::Magma:  StageMagmaManager_Initialize(info);  break;
        }

        g_inited = true;
    }

    static void UpdateCurrent(double dt)
    {
        if (!g_inited) return;

        switch (g_implCur)
        {
        case StageImpl::Simple: StageSimpleManager_Update(dt); break;
        case StageImpl::Magma:  StageMagmaManager_Update(dt);  break;
        }
    }

    static void DrawCurrent()
    {
        if (!g_inited) return;

        switch (g_implCur)
        {
        case StageImpl::Simple: StageSimpleManager_Draw(); break;
        case StageImpl::Magma:  StageMagmaManager_Draw();  break;
        }
    }
}

void StageSystem_Initialize(StageId first)
{
    if (!StageId_IsPlayable(first))
        first = StageId::StageSimple;

    FinalizeCurrent();

    g_cur = first;
    g_req = first;
    g_hasReq = false;

    InitializeStage(g_cur);
}

void StageSystem_Finalize()
{
    FinalizeCurrent();
}

void StageSystem_RequestChange(StageId next)
{
    if (!StageId_IsPlayable(next))
        return;

    g_req = next;
    g_hasReq = true;
}

void StageSystem_RequestNext()
{
    const int cur = static_cast<int>(g_cur);
    const int next = (cur + 1) % kPlayableStageCount;
    StageSystem_RequestChange(static_cast<StageId>(next));
}

void StageSystem_RequestPrev()
{
    const int cur = static_cast<int>(g_cur);
    const int prev = (cur + kPlayableStageCount - 1) % kPlayableStageCount;
    StageSystem_RequestChange(static_cast<StageId>(prev));
}

void StageSystem_Update(double dt)
{
    if (g_hasReq)
    {
        g_hasReq = false;

        if (g_req != g_cur)
        {
            FinalizeCurrent();
            g_cur = g_req;
            InitializeStage(g_cur);
        }
    }

    UpdateCurrent(dt);
}

void StageSystem_Draw()
{
    DrawCurrent();
}

StageId StageSystem_GetCurrent()
{
    return g_cur;
}
