/*==============================================================================

	ゲーム本体[game.h]
														 Author : Tanaka Kouki
														 Date   : 2026/01/01
--------------------------------------------------------------------------------

==============================================================================*/

//ゲームを全て関数で動くようにする
#include "game.h"
#include"staga_system.h"

void Game_Initialize()
{
	StageSystem_Initialize(StageId::Stage1);
}

void Game_Finalize()
{
	StageSystem_Finalize();
}

void Game_Update(float elapsedTime)
{
	StageSystem_Update(elapsedTime);
}

void Game_Draw()
{
	StageSystem_Draw();
}