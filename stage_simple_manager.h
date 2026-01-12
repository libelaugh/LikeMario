/*==============================================================================

　　  ステージsimple管理[stage_simple_manager.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_SIMPLE_MANAGER_H
#define STAGE_SIMPLE_MANAGER_H

#include <DirectXMath.h>

void StageSimpleManager_Initialize();
void StageSimpleManager_Finalize();
void StageSimpleManager_Update(double elapsedTime);
void StageSimpleManager_Draw();

#endif//STAGE_SIMPLE_MANAGER_H