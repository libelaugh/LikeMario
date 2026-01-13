/*==============================================================================

　　  ステージ管理[stage_system.h]
                                                         Author : Kouki Tanaka
                                                         Date   : 2026/01/12
--------------------------------------------------------------------------------

==============================================================================*/

#ifndef STAGE_SYSTEM_H
#define STAGE_SYSTEM_H

#include "stage_registry.h"

void StageSystem_Initialize(StageId first);
void StageSystem_Finalize();
void StageSystem_RequestChange(StageId next);
void StageSystem_RequestNext();
void StageSystem_RequestPrev();
void StageSystem_Update(double dt);
void StageSystem_Draw();
StageId StageSystem_GetCurrent();

#endif//STAGE_SYSTEM_H