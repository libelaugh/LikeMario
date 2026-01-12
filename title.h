/*==============================================================================

　　　タイトルシーン[title.h]
														 Author : Tanaka Kouki
														 Date   : 2026/01/11
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef TITLE_H
#define TITLE_H

#include "stage_registry.h"

void Title_Initialize();
void Title_Finalize();
void Title_Update(double elapsed_time);
void Title_Draw();
bool Title_IsFinished();
StageId Title_GetSelectedStage();


#endif//TITLE_H