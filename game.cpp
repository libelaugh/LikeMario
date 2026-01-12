/*==============================================================================

	ゲーム本体[game.h]
														 Author : Tanaka Kouki
														 Date   : 2026/01/01
--------------------------------------------------------------------------------

==============================================================================*/

//ゲームを全て関数で動くようにする
#include "game.h"
#include"stage_simple_manager.h"
#include"stage_magma_manager.h"

void Game_Initialize()
{
	StageSimpleManager_Initialize();
}

void Game_Finalize()
{
	StageSimpleManager_Finalize();
}

void Game_Update(float elapsedTime)
{
	StageSimpleManager_Update(elapsedTime);
}

void Game_Draw()
{
	StageSimpleManager_Draw();
}