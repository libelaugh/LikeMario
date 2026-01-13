/*==============================================================================

	ゲーム本体[game.h]
														 Author : Tanaka Kouki
														 Date   : 2026/01/01
--------------------------------------------------------------------------------

==============================================================================*/

//ゲームを全て関数で動くようにする

#include "game.h"
#include "staga_system.h"
#include "title.h"
#include "stage_registry.h"

namespace
{
    // true: StageSystem is running, false: Title is running
    bool g_stageInitialized = false;
}

void Game_Initialize()
{
    Title_Initialize();
    g_stageInitialized = false;
}

void Game_Finalize()
{
    if (g_stageInitialized)
    {
        StageSystem_Finalize();
        g_stageInitialized = false;
    }

    Title_Finalize();
}

// ----
// Central stage switch API.
// - StageId::Title: return to title
// - Playable StageId: start/switch stage
// ----
void Game_ChangeStage(StageId next)
{
    // Return to title
    if (next == StageId::Title)
    {
        if (g_stageInitialized)
        {
            StageSystem_Finalize();      //ステージ破棄
            g_stageInitialized = false;  //タイトルへ戻すフラグ
        }

        Title_Initialize();              //タイトル初期化（2周目もここが走る）
        return;
    }

    // Ignore UI-only ids
    if (!StageId_IsPlayable(next))
        return;

    // From title -> stage
    if (!g_stageInitialized)
    {
        Title_Finalize();
        StageSystem_Initialize(next);    //毎回ここでステージ初期化
        g_stageInitialized = true;
        return;
    }

    // From stage -> stage
    StageSystem_RequestChange(next);
}

void Game_Update(float elapsedTime)
{
    if (!g_stageInitialized)
    {
        Title_Update(elapsedTime);

        if (Title_IsFinished())
        {
            const StageId next = Title_GetSelectedStage();
            Game_ChangeStage(next);
        }
        return;
    }

    StageSystem_Update(elapsedTime);
}

void Game_Draw()
{
    if (!g_stageInitialized)
    {
        Title_Draw();
        return;
    }

    StageSystem_Draw();
}