/*==============================================================================

    ゲーム本体[game.h]
														 Author : Youhei Sato
														 Date   : 2025/06/27
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef GAME_H
#define GAME_H
#include "stage_registry.h"
#include<DirectXMath.h>

void Game_Initialize();
void Game_Finalize();

//ゲームにはUpdateとDrawがまず必要
void Game_Update(float elapsed_time);
void Game_Draw();

void Game_ChangeStage(StageId next);


#endif//GAME_H